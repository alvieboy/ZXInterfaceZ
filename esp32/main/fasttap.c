#include "fasttap.h"
#include "esp_log.h"
#include "fileaccess.h"
#include <fcntl.h>
#include <unistd.h>
#include "fpga.h"
#include "model.h"
#include "tzx.h"
#include "log.h"
#include "rom_hook.h"

#define FASTTAP "FASTTAP"

#define FASTTAP_ADDRESS_STATUS 0x028000
#define FASTTAP_ADDRESS_LENLSB 0x028001
#define FASTTAP_ADDRESS_LENMSB 0x028002
#define FASTTAP_ADDRESS_DATA   0x028003

static int fasttap_fd = -1;
static int fasttap_size= -1;
static struct tzx *fasttap_tzx;
static int8_t hooks[3] = {-1};
static int fasttap__install_hooks(model_t model)
{
    uint8_t rom = 0;
    switch (model) {
    case MODEL_16K: /* Fall-through */
    case MODEL_48K:
        ESP_LOGI(FASTTAP, "Hooking to ROM0");
        rom = 0;
        break;
    case MODEL_128K:
        ESP_LOGI(FASTTAP, "Hooking to ROM1");
        rom = 1;
        break;
    default:
        ESP_LOGE(FASTTAP, "No hooks available for model %d", (int)model);
        return -1;
        break;
    }

    hooks[0] = rom_hook__add_pre_set_ranged( rom, 0x056C, 2); // LD-START
    hooks[1] = rom_hook__add_pre_set_ranged( rom, 0x059E, 1);   // "RET NC" on LD-SYNC, set NOP
    hooks[2] = rom_hook__add_pre_set_ranged( rom, 0x05C8, 17);  // LD-MARKER

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

static int fasttap__prepare_tap();
static int fasttap__prepare_tzx();
static int fasttap__load_next_block_tzx();

int fasttap__prepare(const char *filename)
{
    struct stat st;
    int r;
    const char *ext = get_file_extension(filename);

    if (!ext)
        return -1;

    if (fasttap_fd>=0)
        close(fasttap_fd);

    if (fasttap_tzx) {
        free(fasttap_tzx);
        fasttap_tzx = NULL;
    }

    fasttap_fd = __open(filename, O_RDONLY);

    if (fasttap_fd<0) {
        ESP_LOGE(FASTTAP, "Cannot open tape file");
        return -1;
    }

    if (fstat(fasttap_fd, &st)<0) {
        ESP_LOGE(FASTTAP, "Cannot stat tape file");
        close(fasttap_fd);
        fasttap_fd=-1;
        return -1;
    }

    fasttap_size  = st.st_size;

    if (ext) {
        if (ext_match(ext,"tzx")) {
            r = fasttap__prepare_tzx();
        }
        else if (ext_match(ext,"tap")) {
            r = fasttap__prepare_tap();
        } else {
            r = -1;
        }
    }
    if (r<0) {
        close(fasttap_fd);
        fasttap_fd = -1;
    }
    return r;
}

static int fasttap__file_finished()
{
    if (fasttap_fd<0) {
        ESP_LOGI(FASTTAP, "No FASTTAP file loaded");
        return -1;
    }

    unsigned pos = (unsigned)lseek(fasttap_fd, 0, SEEK_CUR);

    ESP_LOGI(FASTTAP, "Compare pos, current %d max %d", pos, fasttap_size);

    if (pos < fasttap_size)
        return 1;

    return 0;
}

int fasttap__is_playing(void)
{
    int r = fasttap__file_finished();
    if (r<0)
        return 0;
    return r;
}

static int fasttap__prepare_tap()
{
    if (fasttap__install_hooks( model__get() )<0)
        return -1;

    ESP_LOGI(FASTTAP,"Tape ready to fast load");
    return 0;
}

/*
 13 00
       00
       00 73 6b 6f 6f 6c 64 61 7a 65 20 1b 00 0a 00 1b 00 44
       1d 00             */
static int fasttap__load_next_block_tap()
{
    uint16_t len;
    uint8_t type;
    if (fasttap_fd<0)
        return -1;

    if (read(fasttap_fd, &len, 2)<2)
        return -1;

    ESP_LOGI(FASTTAP, "Block len %d", len);

    if (read(fasttap_fd, &type, 1)<1)
        return -1;

    ESP_LOGI(FASTTAP, "Block type %d", type);

    len -= 1; // Skip checksum

    int r = fpga__write_extram_block_from_file(FASTTAP_ADDRESS_DATA, fasttap_fd, len, false);
    if (r<0) {
        ESP_LOGE(FASTTAP, "Cannot read TAP file");
        close(fasttap_fd);
        fasttap_fd = -1;
        return -1;
    }

    fpga__write_extram_block(FASTTAP_ADDRESS_LENLSB, (uint8_t*)&len, 2);
    // Mark ready
    ESP_LOGI(FASTTAP,"Block ready");

    if (lseek(fasttap_fd,0,SEEK_CUR)==fasttap_size) {
        fasttap__stop();
    }

    return 0;
}

int fasttap__next()
{
    int r;
    if (fasttap_tzx) {
        r = fasttap__load_next_block_tzx();
    } else {
        r = fasttap__load_next_block_tap();
    }
    fpga__write_extram(FASTTAP_ADDRESS_STATUS, r==0 ? 0x01 : 0xff);
    return r;
}

void fasttap__stop()
{
    if (fasttap_fd<0)
        return;
    ESP_LOGI(FASTTAP, "Fast TAP play finished");
    close(fasttap_fd);
    fasttap_fd=-1;
    if (fasttap_tzx) {
        free(fasttap_tzx);
        fasttap_tzx = NULL;
    }
    fasttap__remove_hooks();
}

/* TZX version */

static int tzx_error;
static int tzx_block_done;
static uint16_t expected_size;
static uint16_t tzx_offset;
static uint16_t tzx_block_size;

static void fasttap__tzx_standard_block_callback(uint16_t length, uint16_t pause_after)
{
    expected_size = length;
    tzx_block_size= length;
    tzx_offset = 0;
}

static void fasttap__tzx_turbo_block_callback(uint16_t pilot,
                                              uint16_t sync0,
                                              uint16_t sync1,
                                              uint16_t pulse0,
                                              uint16_t pulse1,
                                              uint16_t pilot_len,
                                              uint16_t gap_len,
                                              uint32_t data_len,
                                              uint8_t last_byte_len)
{
    tzx_error = 1;
}

static void fasttap__tzx_pure_data_callback(uint16_t pulse0, uint16_t pulse1, uint32_t data_len, uint16_t gap,
                                    uint8_t last_byte_len)
{
    tzx_error = 1;
}

static void fasttap__tzx_data_callback(const uint8_t *data, int len)
{
    if (len<=expected_size) {
        // We need to skip type and final checksum.
        if (tzx_offset==0) {
            tzx_offset++;
            expected_size--;
            len--;
            data++;
        }
        if (len>0) {
            //ESP_LOGI(FASTTAP, "Emmiting %d bytes at offset %d", len, tzx_offset-1);
            //BUFFER_LOGI(FASTTAP, "data", data, len);
            fpga__write_extram_block(FASTTAP_ADDRESS_DATA + (unsigned)tzx_offset-1, data, len);
            expected_size -= len;
            tzx_offset+=len;
        }
    } else {
        ESP_LOGE(FASTTAP,"Data too big for block!");
    }

}

void fasttap__tzx_tone_callback(uint16_t t_states, uint16_t count)
{
    tzx_error = 1;
}

void fasttap__tzx_pulse_callback(uint8_t count, const uint16_t *t_states /* these are words */)
{
    tzx_error = 1;
}

void fasttap__tzx_data_finished_callback(void)
{
    ESP_LOGI(FASTTAP,"Data finished");
    // expected_size should be zero
    if(expected_size>0)
        ESP_LOGE(FASTTAP,"Expected size greater than actual size!");

    tzx_block_size -= 2;
    fpga__write_extram_block(FASTTAP_ADDRESS_LENLSB, (uint8_t*)&tzx_block_size, 2);

    tzx_block_done = 1;
    tzx_offset = 0;
}

void fasttap__tzx_finished_callback(void)
{
    fasttap__stop();
}

const struct tzx_callbacks fasttap_tzx_callbacks =
{
    .standard_block_callback = fasttap__tzx_standard_block_callback,
    .turbo_block_callback    = fasttap__tzx_turbo_block_callback,
    .data_callback           = fasttap__tzx_data_callback,
    .tone_callback           = fasttap__tzx_tone_callback,
    .pulse_callback          = fasttap__tzx_pulse_callback,
    .pure_data_callback      = fasttap__tzx_pure_data_callback,
    .data_finished_callback  = fasttap__tzx_data_finished_callback,
    .finished_callback       = fasttap__tzx_finished_callback
};


static int fasttap__prepare_tzx()
{
    int r = tzx__can_fastplay_fd(fasttap_fd);
    if (r<0) {
        return r;
    }
    if (r!=0) {
        return -1;
    }
    lseek(fasttap_fd,0,SEEK_SET);

    fasttap_tzx = malloc(sizeof(struct tzx));
    if (fasttap_tzx==NULL) {
        return -1;
    }

    tzx__init(fasttap_tzx,&fasttap_tzx_callbacks);

    if (fasttap__install_hooks( model__get() )<0)
        return -1;

    ESP_LOGI(FASTTAP,"Tape ready to fast load (TZX)");
    return 0;
}

static int fasttap__load_next_block_tzx()
{
    uint8_t buf[4]; // This *MUST* be smaller than the minimum TZX block size
    int r;
    tzx_block_done = 0;
    ESP_LOGI(FASTTAP,"TZX fast load next block");
    do {
        r = read(fasttap_fd, buf, sizeof(buf));
        if (r<0) {
            return -1;
        }
      //  ESP_LOGI(FASTTAP,"TZX fast load parse chunk");
        tzx__chunk(fasttap_tzx, buf, r);
    } while (!tzx_block_done);
    ESP_LOGI(FASTTAP,"TZX fast load block done");
    return 0;
}
