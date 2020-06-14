#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "fpga.h"
#include "esp_system.h"
#include "esp_log.h"
#include "defs.h"
#include "tapplayer.h"
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "fileaccess.h"
#include "tzx.h"

#define TAP_CMD_STOP 0
#define TAP_CMD_PLAY 1
#define TAP_CMD_RECORD 2

static xQueueHandle tap_evt_queue = NULL;

static struct tzx tzx;

struct tapcmd
{
    int cmd;
    char file[128];
};

enum tapstate {
    TAP_IDLE,
    TAP_PLAYING,
    TAP_RECORDING,
    TAP_WAITDRAIN
};

static enum tapstate state = TAP_IDLE;
static int tapfh = -1;
static int playsize;
static int currplay;
static bool is_tzx = false;


void tzx__standard_block_callback(uint16_t length, uint16_t pause_after)
{
    uint8_t txbuf[6];
    int i = 0;
    txbuf[i++] = 0x06;
    txbuf[i++] = 0;
    txbuf[i++] = 0;
    txbuf[i++] = 0x09;
    txbuf[i++] = pause_after & 0xff;
    txbuf[i++] = pause_after >> 8;

    fpga__load_tap_fifo_command(txbuf, i, 1000);
}

void tzx__turbo_block_callback(uint16_t pilot, uint16_t sync0, uint16_t sync1, uint16_t pulse0, uint16_t pulse1, uint16_t data_len)
{
    uint8_t txbuf[15];
    int i = 0;
    txbuf[i++] = 0x01;
    txbuf[i++] = pilot & 0xff;
    txbuf[i++] = pilot>>8;
    txbuf[i++] = 0x02;
    txbuf[i++] = sync0 & 0xff;
    txbuf[i++] = sync0>>8;
    txbuf[i++] = 0x03;
    txbuf[i++] = sync1 & 0xff;
    txbuf[i++] = sync1>>8;
    txbuf[i++] = 0x04;
    txbuf[i++] = pulse0 & 0xff;
    txbuf[i++] = pulse0>>8;
    txbuf[i++] = 0x05;
    txbuf[i++] = pulse1 & 0xff;
    txbuf[i++] = pulse1>>8;

    fpga__load_tap_fifo_command(txbuf, i, 1000);
}

void tzx__data_callback(const uint8_t *data, int len)
{
    fpga__load_tap_fifo(data, len, 1000);
}


void tapplayer__stop()
{
    struct tapcmd cmd;
    cmd.cmd = TAP_CMD_STOP;
    cmd.file[0] = '\0';
    xQueueSend(tap_evt_queue, &cmd, 1000);
}

void tapplayer__play(const char *filename)
{
    struct tapcmd cmd;
    cmd.cmd = TAP_CMD_PLAY;
    fullpath(filename, &cmd.file[0], sizeof(cmd.file)-1);
    xQueueSend(tap_evt_queue, &cmd, 1000);
}

static void tapplayer__do_start_play(const char *filename)
{
    struct stat st;

    switch (state) {
    case TAP_IDLE:
        break;
    case TAP_RECORDING:
        /* Fall-through */
    case TAP_PLAYING:
        // Stop tape first.
        if (tapfh>=0) {
            close(tapfh);
            tapfh = -1;
        }
        break;
    case TAP_WAITDRAIN:
        return; // Don't
    }
    // Attempt to open file
    tapfh = open(filename, O_RDONLY);
    is_tzx = false;


    if (tapfh<0) {
        ESP_LOGE(TAG,"Cannot open '%s': %s", filename, strerror(errno));
        return;
    }

    if (fstat(tapfh, &st)<0) {
        ESP_LOGE(TAG,"Cannot stat '%s': %s", filename, strerror(errno));
        return;
    }

    const char *ext = get_file_extension(filename);
    if (ext) {
        if (ext_match(ext,"tzx")) {
            is_tzx = true;
            tzx__init(&tzx);
        }
    }

    playsize = st.st_size;
    currplay = 0;

    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET| FPGA_FLAG_TAP_ENABLED | FPGA_FLAG_ULAHACK);
    fpga__clear_flags(FPGA_FLAG_TAPFIFO_RESET);

    ESP_LOGI(TAG, "Starting TAP play of '%s'\n", filename);
    // Fill in buffers
    state = TAP_PLAYING;
}

static void tapplayer__do_stop()
{
    if (tapfh) {
        close(tapfh);
        tapfh = -1;
    }
    fpga__clear_flags(FPGA_FLAG_TAP_ENABLED);// | FPGA_FLAG_ULAHACK);
    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET);
    state = TAP_IDLE;
}

static void tapplayer__do_record()
{
}

static void do_play_tap()
{
    uint8_t chunk[256];
    int r;

    if (currplay < playsize) {
        unsigned av = fpga__get_tap_fifo_free();
        if (av==0) {
            ESP_LOGW(TAG,"No room");
            return;
        }
        if (av>sizeof(chunk)) {
            av = sizeof(chunk);
        }
        if (av>(playsize-currplay)) {
            av = playsize-currplay;
        }
        r = read(tapfh, chunk, av);
        if (r<0) {
            // Error reading
            ESP_LOGE(TAG,"Read error");
            state = TAP_IDLE;
            close(tapfh);
            tapfh = -1;
            return;
        }
        ESP_LOGI(TAG, "Write TAP chunk %d (%d, %d)",r, playsize, currplay);
        fpga__load_tap_fifo(chunk,r,4000);
        currplay+=r;
        if (playsize==currplay) {
            ESP_LOGI(TAG, "Finished TAP fill");
            close(tapfh);
            tapfh = -1;
            state = TAP_WAITDRAIN;
        }
    }
}

static void do_play_tzx()
{
    uint8_t chunk[32];
    int r;

    if (currplay < playsize) {
        unsigned av = fpga__get_tap_fifo_free();

        if (av>sizeof(chunk)) {
            av = sizeof(chunk);
        }
        if (av>(playsize-currplay)) {
            av = playsize-currplay;
        }
        if (av>0) {
            r = read( tapfh, chunk, av);
            if (r<0) {
                // Error reading
                ESP_LOGE(TAG,"Read error");
                state = TAP_IDLE;
                close(tapfh);
                tapfh = -1;
                return;
            }
            tzx__chunk(&tzx, chunk, r);
        }
    }
}

static void tapplayer__task(void*data)
{
    struct tapcmd cmd;
    int r;

    while (1) {
        if (xQueueReceive(tap_evt_queue, &cmd, 50)==pdTRUE) {

            switch (cmd.cmd) {
            case TAP_CMD_PLAY:
                tapplayer__do_start_play(cmd.file);
                break;
            case TAP_CMD_STOP:
                tapplayer__do_stop();
                break;
            case TAP_CMD_RECORD:
                tapplayer__do_record();
                break;
            }

        } else {
            // No data
            switch (state) {
            case TAP_PLAYING:
                if (tapfh<0) {
                    ESP_LOGW(TAG,"Negative FD! Cannot play!");
                    state = TAP_IDLE;
                } else {
                    if (is_tzx) {
                        do_play_tzx();
                    } else {
                        do_play_tap();
                    }
                }
                break;
            case TAP_IDLE:
                break;
            case TAP_WAITDRAIN:
                if (fpga__tap_fifo_empty()) {
                    ESP_LOGI(TAG, "Finished TAP play");
                    fpga__clear_flags(FPGA_FLAG_TAP_ENABLED);// | FPGA_FLAG_ULAHACK);
                    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET);
                    state = TAP_IDLE;
                }
            default:
                break;
            }
        }
    }
}


void tapplayer__init()
{
    tap_evt_queue = xQueueCreate(2, sizeof(struct tapcmd));

    xTaskCreate(tapplayer__task, "tapplayer_task", 4096, NULL, 10, NULL);
}
