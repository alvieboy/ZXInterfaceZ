#include "fasttap.h"
#include "esp_log.h"
#include "fileaccess.h"
#include <fcntl.h>
#include <unistd.h>
#include "fpga.h"
#include "model.h"

#define FASTTAP "FASTTAP"

#define FASTTAP_ADDRESS_STATUS 0x028000
#define FASTTAP_ADDRESS_LENLSB 0x028001
#define FASTTAP_ADDRESS_LENMSB 0x028002
#define FASTTAP_ADDRESS_DATA   0x028003

static int fasttap_fd = -1;
static int fasttap_size= -1;

static int fasttap__install_hooks(model_t model)
{
    switch (model) {
    case MODEL_16K: /* Fall-through */
    case MODEL_48K:
        ESP_LOGI(FASTTAP, "Hooking to ROM0");
        fpga__enable_hook(0, HOOK_ROM0(0x56C), 2);   // LD-START
        fpga__enable_hook(1, HOOK_ROM0(0x59E), 1);   // "RET NC" on LD-SYNC, set NOP
        fpga__enable_hook(2, HOOK_ROM0(0x5C8), 17);  // LD-MARKER
        break;
    case MODEL_128K:
        ESP_LOGI(FASTTAP, "Hooking to ROM1");
        fpga__enable_hook(0, HOOK_ROM1(0x56C), 2);   // LD-START
        fpga__enable_hook(1, HOOK_ROM1(0x59E), 1);   // "RET NC" on LD-SYNC, set NOP
        fpga__enable_hook(2, HOOK_ROM1(0x5C8), 17);  // LD-MARKER
        break;
    default:
        ESP_LOGE(FASTTAP, "No hooks available for model %d", (int)model);
        return -1;
        break;
    }
    return 0;
}

static void fasttap__remove_hooks()
{
    fpga__disable_hooks();
}

int fasttap__prepare(const char *filename)
{
    struct stat st;
    if (fasttap_fd>=0)
        close(fasttap_fd);

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
static int fasttap__load_next_block()
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
    int r =  fasttap__load_next_block();
    fpga__write_extram(FASTTAP_ADDRESS_STATUS, r==0 ? 0x01 : 0xff);
    return r;
}

void fasttap__stop()
{
    if (fasttap_fd<0)
        return;

    close(fasttap_fd);
    fasttap_fd=-1;
    fasttap__remove_hooks();
}
