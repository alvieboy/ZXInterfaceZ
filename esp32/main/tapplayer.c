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

#define TAP_CMD_STOP 0
#define TAP_CMD_PLAY 1
#define TAP_CMD_RECORD 2

static xQueueHandle tap_evt_queue = NULL;

struct tapcmd
{
    int cmd;
    char file[128];
};

enum tapstate {
    TAP_IDLE,
    TAP_PLAYING,
    TAP_RECORDING
};

static enum tapstate state = TAP_IDLE;
static int tapfh = -1;
static int playsize;
static int currplay;

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
    strcpy(&cmd.file[0], filename);
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
    }
    // Attempt to open file
    tapfh = open(filename, O_RDONLY);

    if (tapfh<0) {
        ESP_LOGE(TAG,"Cannot open '%s': %s", filename, strerror(errno));
        return;
    }

    if (fstat(tapfh, &st)<0) {
        ESP_LOGE(TAG,"Cannot stat '%s': %s", filename, strerror(errno));
        return;
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
    fpga__clear_flags(FPGA_FLAG_TAP_ENABLED | FPGA_FLAG_ULAHACK);
    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET);
    state = TAP_IDLE;
}

static void tapplayer__do_record()
{
}


static void tapplayer__task(void*data)
{
    struct tapcmd cmd;
    uint8_t chunk[256];
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
                    if (currplay < playsize) {
                        unsigned av = fpga__get_tap_fifo_free();
                        if (av==0) {
                            ESP_LOGW(TAG,"No room");
                            break;
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
                            break;
                        }
                        ESP_LOGI(TAG, "Write TAP chunk %d",r);
                        fpga__load_tap_fifo(chunk,r,4000);
                        playsize+=r;
                        if (playsize==currplay) {
                            ESP_LOGI(TAG, "Finished TAP play");
                            close(tapfh);
                            tapfh = -1;
                            state = TAP_IDLE;
                        }
                    }
                }
                break;
            case TAP_IDLE:
                break;
            default:
                break;
            }
        }
    }


#if 0
    ESP_LOGI(TAG, "Starting TAP file play len %d", tapfile_len);

    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET| FPGA_FLAG_TAP_ENABLED | FPGA_FLAG_ULAHACK);
    fpga__clear_flags(FPGA_FLAG_TAPFIFO_RESET);

    int len = tapfile_len;
    const uint8_t *tap = &tapfile[0];
    int chunk;

    do {
        if (len>0) {
            chunk = fpga__load_tap_fifo(tap, len, 1000);
            //ESP_LOGI(TAG, "TAP: Loaded %d bytes", chunk);
            if (chunk<0){
                ESP_LOGW(TAG, "Cannot write to TAP fifo");
            } else {
                len-=chunk;
                tap+=chunk;
                if (len==0) {
                    ESP_LOGI(TAG,"TAP play finished");
                }
            }
        } 
        vTaskDelay(100 / portTICK_RATE_MS);
    } while (1);
#endif
}


void tapplayer__init()
{
    tap_evt_queue = xQueueCreate(2, sizeof(struct tapcmd));

    xTaskCreate(tapplayer__task, "tapplayer_task", 4096, NULL, 10, NULL);
}
