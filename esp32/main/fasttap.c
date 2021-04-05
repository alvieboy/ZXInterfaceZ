/**
 * \defgroup fasttap Fast TAP
 * \brief Fast TAP loading routines
 *
 * The Fast TAP infrastructure uses objects to handle Fast TAP loading, depending on the file type.
 *
 * There are currently three Fast TAP implementations:
 *
 * - fasttap_tap
 * - fasttap_tzx
 * - fasttap_scr
 *
 * Each of them is implemented in a separate file for better organization.
 *
 */
#include "fasttap.h"
#include "esp_log.h"
#include "fileaccess.h"
#include <fcntl.h>
#include <unistd.h>
#include "fpga.h"
#include "model.h"
#include "log.h"
#include "rom_hook.h"
#include "memlayout.h"
#include "fasttap_tap.h"
#include "fasttap_tzx.h"
#include "fasttap_scr.h"
#include "stream.h"

#define FASTTAP "FASTTAP"

static int8_t hooks[3] = {-1};
static fasttap_t *fasttap_instance = NULL;

/**
 * \ingroup fasttap
 * \brief
 */

/**
 * \ingroup fasttap
 * \brief Inititalise the Fast TAP subsystem
 */
void fasttap__init()
{
    hooks[0] = -1;
    hooks[1] = -1;
    hooks[2] = -1;
}

/**
 * \ingroup fasttap
 * \brief Install Fast TAP hooks for a particular model
 * \param model The ZX Spectrum model
 * \return 0 if successful
 */

int fasttap__install_hooks(model_t model)
{
    uint8_t rom = 0;
    switch (model) {
    case MODEL_16K: /* Fall-through */
    case MODEL_48K:
        ESP_LOGI(FASTTAP, "Hooking to ROM0");
        rom = 0;
        break;
    case MODEL_128K: /* Fall-through */
    case MODEL_2APLUS:/* Fall-through */
    case MODEL_3A:
        ESP_LOGI(FASTTAP, "Hooking to ROM1");
        rom = 1;
        break;
    default:
        ESP_LOGE(FASTTAP, "No hooks available for model %d", (int)model);
        return -1;
        break;
    }
    if (hooks[0]<0)
        hooks[0] = rom_hook__add_pre_set_ranged( rom, 0x056C, 2); // LD-START
    if (hooks[1]<0)
        hooks[1] = rom_hook__add_pre_set_ranged( rom, 0x059E, 1);   // "RET NC" on LD-SYNC, set NOP
    if (hooks[2]<0)
        hooks[2] = rom_hook__add_pre_set( rom, 0x05C8, 1);  // LD-MARKER, not ranged

    return 0;
}

/**
 * \ingroup fasttap
 * \brief Remove Fast TAP hooks
 */

static void fasttap__remove_hooks()
{
    int i;
    for (i=0;i<3; i++) {
        rom_hook__remove(hooks[i]);
        hooks[i] = -1;
    }
}

/**
 * \ingroup fasttap
 * \brief Allocate a new Fast TAP for a specified type
 * \param ext The type
 * \return pointer to the new Fast TAP object, or NULL if any error
 */

static fasttap_t *fasttap__allocate_by_type(tap_type_t type)
{
    fasttap_t *tap = NULL;

    switch (type) {
    case TAP_TYPE_TZX:
        tap = fasttap_tzx__allocate();
        break;
    case TAP_TYPE_TAP:
        tap = fasttap_tap__allocate();
        break;
    case TAP_TYPE_SCR:
        tap = fasttap_scr__allocate();
        break;
    default:
        break;
    }

    if (tap==NULL) {
        ESP_LOGE(FASTTAP, "Invalid file extension, cannot load");
    }
    return tap;
}

tap_type_t fasttap__type_from_ext(const char *ext)
{
    tap_type_t type = TAP_TYPE_UNKNOWN;
    if (ext) {
        do {
            if (ext_match(ext,"tzx")) {
                type = TAP_TYPE_TZX;
                break;
            }
            if (ext_match(ext,"tap")) {
                type = TAP_TYPE_TAP;
                break;
            }
            if (ext_match(ext,"scr")) {
                type = TAP_TYPE_SCR;
                break;
            }
        } while (0);
    }
    return type;
}


/**
 * \ingroup fasttap
 * \brief Allocate a new Fast TAP for a specified file extension
 * \param ext The file extension ("tap", "tzx" or "scr")
 * \return pointer to the new Fast TAP object, or NULL if any error
 */

static fasttap_t *fasttap__allocate_by_ext(const char *ext)
{
    tap_type_t type = fasttap__type_from_ext(ext);
    return fasttap__allocate_by_type(type);
}

/**
 * \ingroup fasttap
 * \brief Destroy the previous Fast TAP object.
 */
static void fasttap__destroy_previous()
{
    if (fasttap_instance) {
        ESP_LOGI(FASTTAP, "Destroying stream");
        if (fasttap_instance->stream!=NULL)
            fasttap_instance->stream = stream__destroy(fasttap_instance->stream);
        fasttap_instance->ops->free(fasttap_instance);
        fasttap_instance = NULL;
    }
}

/**
 * \ingroup fasttap
 * \brief Prepare Fast TAP loading from a specified file
 * \param filename The file to load
 * \return 0 if successful
 */

int fasttap__prepare_from_file(const char *filename)
{
    struct stat st;
    const char *ext = get_file_extension(filename);

    if (!ext)
        return -1;

    fasttap__destroy_previous();

    int fasttap_fd = __open(filename, O_RDONLY);

    if (fasttap_fd<0) {
        ESP_LOGE(FASTTAP, "Cannot open tape file");
        return -1;
    }

    if (fstat(fasttap_fd, &st)<0) {
        ESP_LOGE(FASTTAP, "Cannot stat tape file");
        close(fasttap_fd);
        return -1;
    }

    fasttap_instance = fasttap__allocate_by_ext(ext);

    if (!fasttap_instance) {
        ESP_LOGE(FASTTAP, "Could not instantiate FASTTAP engine");
        close(fasttap_fd);
        return -1;
    }

    fasttap_instance->stream = stream__alloc_system(fasttap_fd);
    fasttap_instance->size = st.st_size;
    fasttap_instance->read = 0;

    if (fasttap_instance->ops->init(fasttap_instance)<0) {
        ESP_LOGE(FASTTAP, "Could not prepare FASTTAP engine");
        fasttap__destroy_previous();
        return -1;
    }

    return 0;
}

int fasttap__prepare_from_stream(struct stream *stream, size_t size, tap_type_t type)
{
    fasttap__destroy_previous();

    fasttap_instance = fasttap__allocate_by_type(type);

    if (!fasttap_instance) {
        ESP_LOGE(FASTTAP, "Could not instantiate FASTTAP engine");
        return -1;
    }

    fasttap_instance->stream = stream;
    fasttap_instance->size = size;
    fasttap_instance->read = 0;

    if (fasttap_instance->ops->init(fasttap_instance)<0) {
        ESP_LOGE(FASTTAP, "Could not prepare FASTTAP engine");
        fasttap__destroy_previous();
        return -1;
    }

    return 0;

}


/**
 * \ingroup fasttap
 * \brief Check if we reached EOF on a particular fasttap
 * \param fasttap The FastTAP object
 * \return true if at EOF, false otherwise
 */

bool fasttap__is_file_eof(fasttap_t *fasttap)
{
    bool r = true;
    if (fasttap_instance->read < fasttap_instance->size)
        r = false;

    ESP_LOGI(FASTTAP, "File is eof %d %d: %d", fasttap_instance->read,
             fasttap_instance->size, r);
    /*unsigned pos = (unsigned)lseek(fasttap->fd, 0, SEEK_CUR);

    if (pos < (unsigned)fasttap->size)
        return false;
        */
    return r;
}

/**
 * \ingroup fasttap
 * \brief Check if we have finished playing a particular fasttap
 * \param fasttap The FastTAP object
 * \return true if we finished playing, false otherwise
 */
static bool fasttap__file_finished()
{
    if (fasttap_instance==NULL)
        return true;

    return fasttap_instance->ops->finished(fasttap_instance);
}

/**
 * \ingroup fasttap
 * \brief Check if we are playing any Fast TAP
 * \return true if we are playing, false otherwise
 */
bool fasttap__is_playing(void)
{
    if (fasttap_instance==NULL)
        return false;
    return !fasttap__file_finished();
}


/**
 * \ingroup fasttap
 * \brief Load next block for current fast TAP
 * \param type Type of block expected by Spectrum
 * \param len Length of block requested by Spectrum
 * \return 0 if loaded successfully
 */
int fasttap__next(uint8_t type, uint16_t len)
{
    if (fasttap_instance==NULL)
        return -1;

    int r;

    r = fasttap_instance->ops->next(fasttap_instance, type, len);

    fpga__write_extram(FASTTAP_ADDRESS_STATUS, r==0 ? 0x01 : 0xff);

    return r;
}

/**
 * \ingroup fasttap
 * \brief Stop the current Fast TAP
 */
void fasttap__stop()
{
    if (fasttap_instance==NULL)
        return;

    fasttap_instance->ops->stop(fasttap_instance);

    fasttap__remove_hooks();
    fasttap__destroy_previous();
}

int fasttap__read(fasttap_t *tap, void *buf, size_t size)
{
    int r = stream__read(tap->stream, buf, size);

    if (r>0) {
        ESP_LOGI(FASTTAP," -- Read %d, totoal now %d", r, tap->read);
        tap->read += r;
    }
    return r;
}
