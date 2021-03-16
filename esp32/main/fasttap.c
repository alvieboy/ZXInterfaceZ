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

#define FASTTAP "FASTTAP"

static int8_t hooks[3] = {-1};

static fasttap_t *fasttap_instance = NULL;

void fasttap__init()
{
    hooks[0] = -1;
    hooks[1] = -1;
    hooks[2] = -1;
}

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

static void fasttap__remove_hooks()
{
    int i;
    for (i=0;i<3; i++) {
        rom_hook__remove(hooks[i]);
        hooks[i] = -1;
    }
}

static fasttap_t *fasttap__allocate(const char *ext)
{
    if (ext) {
        if (ext_match(ext,"tzx")) {
            return fasttap_tzx__allocate();
        }
        if (ext_match(ext,"tap")) {
            return fasttap_tap__allocate();
        }
        if (ext_match(ext,"scr")) {
            return fasttap_scr__allocate();
        }
    }
    ESP_LOGE(FASTTAP, "Invalid file extension, cannot load");
    return NULL;
}

/*static int fasttap__prepare_tap();
static int fasttap__prepare_tzx();
static int fasttap__prepare_scr();
static int fasttap__load_next_block_tzx();
static int fasttap__load_next_block_scr();
*/

static void fasttap__destroy_previous()
{
    if (fasttap_instance) {
        if (fasttap_instance->fd >=0)
            close(fasttap_instance->fd);
        fasttap_instance->ops->free(fasttap_instance);
        fasttap_instance = NULL;
    }
}

int fasttap__prepare(const char *filename)
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

    fasttap_instance = fasttap__allocate(ext);

    if (!fasttap_instance) {
        ESP_LOGE(FASTTAP, "Could not instantiate FASTTAP engine");
        close(fasttap_fd);
        return -1;
    }

    fasttap_instance->fd = fasttap_fd;
    fasttap_instance->size = st.st_size;

    if (fasttap_instance->ops->init(fasttap_instance)<0) {
        ESP_LOGE(FASTTAP, "Could not prepare FASTTAP engine");
        fasttap__destroy_previous();
        return -1;
    }

    return 0;

}

bool fasttap__is_file_eof(fasttap_t *fasttap)
{
    unsigned pos = (unsigned)lseek(fasttap->fd, 0, SEEK_CUR);

    if (pos < (unsigned)fasttap->size)
        return false;

    return true;
}

static bool fasttap__file_finished()
{
    if (fasttap_instance==NULL)
        return true;

    return fasttap_instance->ops->finished(fasttap_instance);
}

bool fasttap__is_playing(void)
{
    if (fasttap_instance==NULL)
        return false;
    return !fasttap__file_finished();
}


int fasttap__next(uint8_t type, uint16_t len)
{
    if (fasttap_instance==NULL)
        return -1;

    int r;

    r = fasttap_instance->ops->next(fasttap_instance, type, len);

    fpga__write_extram(FASTTAP_ADDRESS_STATUS, r==0 ? 0x01 : 0xff);

    return r;
}

void fasttap__stop()
{
    if (fasttap_instance==NULL)
        return;

    fasttap_instance->ops->stop(fasttap_instance);

    fasttap__remove_hooks();
    fasttap__destroy_previous();
}


