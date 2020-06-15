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

#define TAP_INTERNALCMD_RESET 0x80
#define TAP_INTERNALCMD_DATALEN_EXTERNAL 0x82
#define TAP_INTERNALCMD_DATALEN_INTERNAL 0x83

#define TAP_INTERNALCMD_SET_PILOT 0x00
#define TAP_INTERNALCMD_SET_SYNC0 0x01
#define TAP_INTERNALCMD_SET_SYNC1 0x02
#define TAP_INTERNALCMD_SET_PULSE0 0x03
#define TAP_INTERNALCMD_SET_PULSE1 0x04
#define TAP_INTERNALCMD_SET_PILOT_HEADER_LEN_PAIRS 0x08
#define TAP_INTERNALCMD_SET_PILOT_DATA_LEN_PAIRS 0x09
#define TAP_INTERNALCMD_SET_GAP 0x0A
#define TAP_INTERNALCMD_SET_DATALEN1 0x0B
#define TAP_INTERNALCMD_SET_DATALEN2 0x0C


void tzx__standard_block_callback(uint16_t length, uint16_t pause_after)
{
    uint8_t txbuf[4];
    int i = 0;
    txbuf[i++] = TAP_INTERNALCMD_RESET;
    txbuf[i++] = TAP_INTERNALCMD_SET_GAP;
    txbuf[i++] = pause_after & 0xff;
    txbuf[i++] = pause_after >> 8;

    fpga__load_tap_fifo_command(txbuf, i, 1000);
}

static inline uint16_t tapplayer__compute_tstate_delay(uint16_t cycles)
{
    /* NOTES ABOUT T-CYCLE COMPENSATION

    The FPGA system runs at 96MHz. With this speed it is not possible to generate a perfect 3.5MHz toggle. The ratio
    is ~27.428571, and internal FPGA uses 27.0 as the clock divider.

    In order to compensate for this small difference, adjust your T-state timings by:

    * Multiplying for 960 and then dividing for 945.

    Example: you want a 2174-cycle delay. 2174*960= 2087040, round(2087040/945) = 2209
    */

    uint32_t temp = cycles;

    // In order to round up, we use a single bit (using fixed-point arithmetic).
    // We first add 0.5 and then truncate the result.
    //
    //  t = truncate((a*b)/c + 0.5)
    //   which is equivalent to
    //  2*t = truncate((a*2*b)/c + 2*0.5)
    //    t = truncate((a*2*b)/c + 1) / 2



    temp *= 960*2; // 4174080 for the above example
    temp /= 945;   // 4417 for the above example
    temp++;        // 4418
    temp>>=1;      // 2209
    return (uint16_t) temp;
}

void tzx__turbo_block_callback(uint16_t pilot, uint16_t sync0, uint16_t sync1, uint16_t pulse0, uint16_t pulse1, uint32_t data_len,
                              uint8_t last_byte_len)
{
    uint8_t txbuf[24];
    int i = 0;

    if (last_byte_len==0)
        last_byte_len = 1;

    if (last_byte_len>8)
        last_byte_len = 8;

    pilot = tapplayer__compute_tstate_delay(pilot);
    sync0 = tapplayer__compute_tstate_delay(sync0);
    sync1 = tapplayer__compute_tstate_delay(sync1);
    pulse0 = tapplayer__compute_tstate_delay(pulse0);
    pulse1 = tapplayer__compute_tstate_delay(pulse1);

    txbuf[i++] = TAP_INTERNALCMD_SET_PILOT;
    txbuf[i++] = pilot & 0xff;
    txbuf[i++] = pilot>>8;
    txbuf[i++] = TAP_INTERNALCMD_SET_SYNC0;
    txbuf[i++] = sync0 & 0xff;
    txbuf[i++] = sync0>>8;
    txbuf[i++] = TAP_INTERNALCMD_SET_SYNC1;
    txbuf[i++] = sync1 & 0xff;
    txbuf[i++] = sync1>>8;
    txbuf[i++] = TAP_INTERNALCMD_SET_PULSE0;
    txbuf[i++] = pulse0 & 0xff;
    txbuf[i++] = pulse0>>8;
    txbuf[i++] = TAP_INTERNALCMD_SET_PULSE1;
    txbuf[i++] = pulse1 & 0xff;
    txbuf[i++] = pulse1>>8;
    txbuf[i++] = TAP_INTERNALCMD_DATALEN_EXTERNAL;  // This informs player that block len is not in stream
    txbuf[i++] = TAP_INTERNALCMD_SET_DATALEN1;
    txbuf[i++] = data_len & 0xff;
    txbuf[i++] = (data_len >> 8) & 0xff;
    txbuf[i++] = TAP_INTERNALCMD_SET_DATALEN2;
    txbuf[i++] = (data_len >> 16) & 0xff;
    txbuf[i++] = (8 - last_byte_len);


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
    tapfh = __open(filename, O_RDONLY);
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
