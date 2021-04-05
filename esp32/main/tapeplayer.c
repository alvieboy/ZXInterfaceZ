#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "fpga.h"
#include "esp_system.h"
#include "esp_log.h"
#include "defs.h"
#include "tapeplayer.h"
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "fileaccess.h"
#include "tap.h"
#include "tzx.h"
#include "byteops.h"
#include "interfacez_tasks.h"
#include "stream.h"

#define TAP_CMD_STOP 0
#define TAP_CMD_PLAY 1
#define TAP_CMD_RECORD 2
#define TAP_CMD_PLAYSTREAM 3

static xQueueHandle tap_evt_queue = NULL;

static union {
    struct tzx tzx;
    struct tap tap;
} tape_data;

struct tapcmd
{
    int cmd;
    union {
        char file[128];
        struct {
            struct stream *stream;
            size_t stream_size;
            bool is_tzx;
            int initial_delay;
        };
    };
};

enum tapstate {
    TAP_IDLE,
    TAP_PLAYING,
    TAP_RECORDING,
    TAP_WAITDRAIN
};

static enum tapstate state = TAP_IDLE;
static struct stream *tap_stream = NULL;
static int playsize;
static int currplay;
static bool is_tzx = false;
static uint32_t gap; // Upcoming gap

#define TAP_INTERNALCMD_SET_LOGIC0 0x80
#define TAP_INTERNALCMD_SET_LOGIC1 0x81
#define TAP_INTERNALCMD_GAP        0x82
#define TAP_INTERNALCMD_SET_DATALEN1 0x83
#define TAP_INTERNALCMD_SET_DATALEN2 0x84
#define TAP_INTERNALCMD_SET_REPEAT 0x85 /* Follows pulse repeat */
#define TAP_INTERNALCMD_PLAY_PULSE 0x86 /* Follows pulse t-states, repeats for REPEAT */
#define TAP_INTERNALCMD_FLUSH 0x87 /* Ignored. Use to flush SPI */

#define TAP_DEFAULT_PILOT_LEN (8063) /* This depends on header/data, use same */

#define TAP_DEFAULT_PILOT_TSTATES 2168
#define TAP_DEFAULT_SYNC0_TSTATES 667
#define TAP_DEFAULT_SYNC1_TSTATES 735
#define TAP_DEFAULT_LOGIC0_TSTATES 855
#define TAP_DEFAULT_LOGIC1_TSTATES 1710

static inline uint16_t tapeplayer__compute_tstate_delay(uint16_t cycles)
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


#ifdef __linux__
    return temp;
#else
    temp *= 960*2; // 4174080 for the above example
    temp /= 945;   // 4417 for the above example
    temp++;        // 4418
    temp>>=1;      // 2209
    return (uint16_t) temp;
#endif
}

static void tapeplayer__do_load_data(const uint8_t *buf, unsigned size)
{
    do {
        int sent = fpga__load_tap_fifo(buf, size, 10000);
        size -= sent;
        buf += sent;
    } while (size>0);
}

static void tapeplayer__do_load_command(const uint8_t *buf, unsigned size)
{
    do {
        int sent = fpga__load_tap_fifo_command(buf, size, 10000);
        size -= sent;
        buf += sent;
    } while (size>0);

}

static void tapeplayer__tzx_tone_callback(void *userdata, uint16_t tstates, uint16_t count)
{
    uint8_t txbuf[6];
    int i = 0;
    //count--;

    txbuf[i++] = TAP_INTERNALCMD_SET_REPEAT;
    txbuf[i++] = count & 0xff;
    txbuf[i++] = count >> 8;

    tstates = tapeplayer__compute_tstate_delay(tstates);

    txbuf[i++] = TAP_INTERNALCMD_PLAY_PULSE;
    txbuf[i++] = tstates & 0xff;
    txbuf[i++] = tstates >> 8;

    tapeplayer__do_load_command(txbuf,i);
}

static void tapeplayer__tzx_pulse_callback(void *userdata, uint8_t count, const uint16_t *t_states /* these are words */)
{
    uint8_t txbuf[3];
    int i = 0;

    txbuf[i++] = TAP_INTERNALCMD_SET_REPEAT;
    txbuf[i++] = 0;
    txbuf[i++] = 0;

    tapeplayer__do_load_command(txbuf, i);
    ESP_LOGI(TAG, "Transmitting pulses (%d)", count);
    // Now, play all pulses. Improve this for speed please
    while (count--) {
        i = 0;
        uint16_t ts = tapeplayer__compute_tstate_delay(*t_states);
        txbuf[i++] = TAP_INTERNALCMD_PLAY_PULSE;
        txbuf[i++] = ts & 0xff;
        txbuf[i++] = ts>>8;
        ESP_LOGI(TAG, " - %d", ts);
        t_states++;
        tapeplayer__do_load_command(txbuf, i);
    }

}

static void tapeplayer__standard_block(uint16_t length, uint16_t pause_after, uint16_t pilot_len)
{
    uint8_t txbuf[32];
    int i = 0;

    uint16_t pulse0 = tapeplayer__compute_tstate_delay(TAP_DEFAULT_LOGIC0_TSTATES);
    uint16_t pulse1 = tapeplayer__compute_tstate_delay(TAP_DEFAULT_LOGIC1_TSTATES);

    length--;

    txbuf[i++] = TAP_INTERNALCMD_SET_LOGIC0;
    txbuf[i++] = pulse0 & 0xff;
    txbuf[i++] = pulse0>>8;

    txbuf[i++] = TAP_INTERNALCMD_SET_LOGIC1;
    txbuf[i++] = pulse1 & 0xff;
    txbuf[i++] = pulse1>>8;

    txbuf[i++] = TAP_INTERNALCMD_SET_DATALEN1;
    txbuf[i++] = length & 0xff;
    txbuf[i++] = (length >> 8) & 0xff;

    txbuf[i++] = TAP_INTERNALCMD_SET_DATALEN2;
    txbuf[i++] = 0x00;
    txbuf[i++] = 0;

    // Send pilot tone
    txbuf[i++] = TAP_INTERNALCMD_SET_REPEAT;
    i += putle16_c(&txbuf[i], pilot_len);

    // Send pilot tone
    txbuf[i++] = TAP_INTERNALCMD_PLAY_PULSE;
    i += putle16_c(&txbuf[i], TAP_DEFAULT_PILOT_TSTATES);

    // Send sync
    txbuf[i++] = TAP_INTERNALCMD_SET_REPEAT;
    txbuf[i++] = 0;
    txbuf[i++] = 0;

    txbuf[i++] = TAP_INTERNALCMD_PLAY_PULSE;
    i += putle16_c(&txbuf[i], TAP_DEFAULT_SYNC0_TSTATES);

    txbuf[i++] = TAP_INTERNALCMD_PLAY_PULSE;
    i += putle16_c(&txbuf[i], TAP_DEFAULT_SYNC1_TSTATES);

    ESP_LOGI(TAG, " Standard block 0=%d 1=%d", pulse0, pulse1);

    gap = pause_after;

    tapeplayer__do_load_command(txbuf, i);
}


void tapeplayer__tzx_standard_block_callback(void *userdata, uint16_t length, uint16_t pause_after)
{
    tapeplayer__standard_block(length, pause_after, TAP_DEFAULT_PILOT_LEN);
}

void tapeplayer__tzx_turbo_block_callback(void *userdata,
                                          uint16_t pilot,
                                          uint16_t sync0,
                                          uint16_t sync1,
                                          uint16_t pulse0,
                                          uint16_t pulse1,
                                          uint16_t pilot_len,
                                          uint16_t gap_len,
                                          uint32_t data_len,
                                          uint8_t last_byte_len)
{
    uint8_t txbuf[32];
    //uint8_t *txptr = &txbuf[0];

    int i = 0;

    if (last_byte_len==0)
        last_byte_len = 1;

    if (last_byte_len>8)
        last_byte_len = 8;

    pilot = tapeplayer__compute_tstate_delay(pilot);
    sync0 = tapeplayer__compute_tstate_delay(sync0);
    sync1 = tapeplayer__compute_tstate_delay(sync1);
    pulse0 = tapeplayer__compute_tstate_delay(pulse0);
    pulse1 = tapeplayer__compute_tstate_delay(pulse1);

    data_len --;
    gap = gap_len;

    txbuf[i++] = TAP_INTERNALCMD_SET_LOGIC0;
    txbuf[i++] = pulse0 & 0xff;
    txbuf[i++] = pulse0>>8;
    txbuf[i++] = TAP_INTERNALCMD_SET_LOGIC1;
    txbuf[i++] = pulse1 & 0xff;
    txbuf[i++] = pulse1>>8;
    txbuf[i++] = TAP_INTERNALCMD_SET_DATALEN1;
    txbuf[i++] = data_len & 0xff;
    txbuf[i++] = (data_len >> 8) & 0xff;
    txbuf[i++] = TAP_INTERNALCMD_SET_DATALEN2;
    txbuf[i++] = (data_len >> 16) & 0xff;
    txbuf[i++] = (8 - last_byte_len);

    // Send pilot tone
    txbuf[i++] = TAP_INTERNALCMD_SET_REPEAT;
    i += putle16_c(&txbuf[i], pilot_len);

    // Send pilot tone
    txbuf[i++] = TAP_INTERNALCMD_PLAY_PULSE;
    i += putle16_c(&txbuf[i], pilot);

    // Send sync
    txbuf[i++] = TAP_INTERNALCMD_SET_REPEAT;
    txbuf[i++] = 0;
    txbuf[i++] = 0;

    txbuf[i++] = TAP_INTERNALCMD_PLAY_PULSE;
    i += putle16_c(&txbuf[i], sync0);

    txbuf[i++] = TAP_INTERNALCMD_PLAY_PULSE;
    i += putle16_c(&txbuf[i], sync1);


    tapeplayer__do_load_command(txbuf, i);
}

void tapeplayer__tzx_pure_data_callback(void *userdata,
                                        uint16_t pulse0,
                                        uint16_t pulse1,
                                        uint32_t data_len,
                                        uint16_t gap_len,
                                        uint8_t last_byte_len)
{
    uint8_t txbuf[17];
    int i = 0;

    pulse0 = tapeplayer__compute_tstate_delay(pulse0);
    pulse1 = tapeplayer__compute_tstate_delay(pulse1);

    gap = gap_len;
    data_len --;

    txbuf[i++] = TAP_INTERNALCMD_SET_LOGIC0;
    txbuf[i++] = pulse0 & 0xff;
    txbuf[i++] = pulse0>>8;
    txbuf[i++] = TAP_INTERNALCMD_SET_LOGIC1;
    txbuf[i++] = pulse1 & 0xff;
    txbuf[i++] = pulse1>>8;
    txbuf[i++] = TAP_INTERNALCMD_SET_DATALEN1;
    txbuf[i++] = data_len & 0xff;
    txbuf[i++] = (data_len >> 8) & 0xff;
    txbuf[i++] = TAP_INTERNALCMD_SET_DATALEN2;
    txbuf[i++] = (data_len >> 16) & 0xff;
    txbuf[i++] = (8 - last_byte_len);

    tapeplayer__do_load_command(txbuf, i);
}


void tap__standard_block_callback(uint16_t length, uint16_t pause_after)
{
    tapeplayer__standard_block(length, pause_after, TAP_DEFAULT_PILOT_LEN);
}

void tapeplayer__tzx_data_callback(void *userdata, const uint8_t *data, int len)
{
    tapeplayer__do_load_data(data,len);
}

static void tapeplayer__data_finished(void)
{
    uint8_t txbuf[3];
    int i = 0;

    txbuf[i++] = TAP_INTERNALCMD_GAP;
    txbuf[i++] = gap & 0xff;
    txbuf[i++] = gap >>8;

    tapeplayer__do_load_command(txbuf, i);
}


void tapeplayer__tzx_data_finished_callback(void *userdata)
{
    tapeplayer__data_finished();
}

static void tapeplayer__finished()
{
    if (tap_stream!=NULL)
        tap_stream = stream__destroy(tap_stream);

    state = TAP_WAITDRAIN;

}

void tapeplayer__tzx_finished_callback(void *userdata)
{
    tapeplayer__finished();
}

void tap__finished_callback()
{
    tapeplayer__finished();
}

void tap__data_finished_callback()
{
    tapeplayer__data_finished();
}

void tap__data_callback(const uint8_t *data, int len)
{
    tapeplayer__do_load_data(data,len);
}

void tapeplayer__stop()
{
    struct tapcmd cmd;
    cmd.cmd = TAP_CMD_STOP;
    cmd.file[0] = '\0';
    xQueueSend(tap_evt_queue, &cmd, 1000);
}

void tapeplayer__play_file(const char *filename)
{
    struct tapcmd cmd;
    cmd.cmd = TAP_CMD_PLAY;
    fullpath(filename, &cmd.file[0], sizeof(cmd.file)-1);
    xQueueSend(tap_evt_queue, &cmd, 1000);
}

void tapeplayer__play_stream(struct stream *s, size_t size, bool is_tzx, int initial_delay)
{
    struct tapcmd cmd;
    cmd.cmd = TAP_CMD_PLAYSTREAM;
    cmd.stream = s;
    cmd.stream_size = size;
    cmd.is_tzx = is_tzx;
    cmd.initial_delay = initial_delay;
    xQueueSend(tap_evt_queue, &cmd, 1000);
}

const struct tzx_callbacks tapeplayer_tzx_callbacks =
{
    .standard_block_callback = tapeplayer__tzx_standard_block_callback,
    .turbo_block_callback    = tapeplayer__tzx_turbo_block_callback,
    .data_callback           = tapeplayer__tzx_data_callback,
    .tone_callback           = tapeplayer__tzx_tone_callback,
    .pulse_callback          = tapeplayer__tzx_pulse_callback,
    .pure_data_callback      = tapeplayer__tzx_pure_data_callback,
    .data_finished_callback  = tapeplayer__tzx_data_finished_callback,
    .finished_callback       = tapeplayer__tzx_finished_callback
};


static void tapeplayer__do_start_play_from_file(const char *filename)
{
    struct stat st;

    switch (state) {
    case TAP_IDLE:
        break;
    case TAP_RECORDING:
        /* Fall-through */
    case TAP_PLAYING:
        // Stop tape first.
        if (tap_stream!=NULL) {
            tap_stream = stream__destroy(tap_stream);
        }
        break;
    case TAP_WAITDRAIN:
        return; // Don't
    }

    // Attempt to open file
    int tapfh = __open(filename, O_RDONLY);
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
            ESP_LOGI(TAG,"Initializing TZX structure");
            tzx__init(&tape_data.tzx, &tapeplayer_tzx_callbacks, NULL);
        } else {
            is_tzx = false;
            ESP_LOGI(TAG,"Initializing TAP structure");
            tap__init(&tape_data.tap);
        }
    }

    tap_stream = stream__alloc_system(tapfh);

    playsize = st.st_size;
    currplay = 0;

    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET| FPGA_FLAG_TAP_ENABLED | FPGA_FLAG_ULAHACK);
    fpga__clear_flags(FPGA_FLAG_TAPFIFO_RESET);

    ESP_LOGI(TAG, "Starting TAP/TZX play of '%s'", filename);
    // Fill in buffers
    state = TAP_PLAYING;
}

static void tapeplayer__do_start_play_from_stream(struct stream *stream, size_t size, bool r_is_tzx, int initial_delay)
{
    struct stat st;

    switch (state) {
    case TAP_IDLE:
        break;
    case TAP_RECORDING:
        /* Fall-through */
    case TAP_PLAYING:
        // Stop tape first.
        if (tap_stream!=NULL) {
            tap_stream = stream__destroy(tap_stream);
        }
        break;
    case TAP_WAITDRAIN:
        return; // Don't
    }

    is_tzx = r_is_tzx;
    if (r_is_tzx) {
        ESP_LOGI(TAG,"Initializing TZX structure");
        tzx__init(&tape_data.tzx, &tapeplayer_tzx_callbacks, NULL);
        tzx__set_initial_delay(&tape_data.tzx, initial_delay);
    } else {
        ESP_LOGI(TAG,"Initializing TAP structure");
        tap__init(&tape_data.tap);
        tap__set_initial_delay(&tape_data.tap, initial_delay);
    }

    tap_stream = stream;

    playsize = size;
    currplay = 0;

    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET| FPGA_FLAG_TAP_ENABLED | FPGA_FLAG_ULAHACK);
    fpga__clear_flags(FPGA_FLAG_TAPFIFO_RESET);

    ESP_LOGI(TAG, "Starting stream TAP/TZX play");
    // Fill in buffers

    state = TAP_PLAYING;
}

static void tapeplayer__do_stop()
{
    if (tap_stream) {
        tap_stream = stream__destroy(tap_stream);
    }
    fpga__clear_flags(FPGA_FLAG_TAP_ENABLED | FPGA_FLAG_ULAHACK);
    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET);
    state = TAP_IDLE;
}

static void tapeplayer__do_record()
{
}

static void do_play_tap()
{
    uint8_t chunk[32];
    int r;

    unsigned av = fpga__get_tap_fifo_free();

    if (av>sizeof(chunk)) {
        av = sizeof(chunk);
    }
    if (av>(playsize-currplay)) {
        av = playsize-currplay;
    }

    if (av>0) {
        r = stream__read( tap_stream, chunk, av);
        if (r<0) {
            // Error reading
            ESP_LOGE(TAG,"Read error");
            state = TAP_IDLE;
            tap_stream = stream__destroy(tap_stream);
            return;
        }
        tap__chunk(&tape_data.tap, chunk, r);
    }
}

static void do_play_tzx()
{
    uint8_t chunk[32];
    int r;

    unsigned av = fpga__get_tap_fifo_free();

    if (av>sizeof(chunk)) {
        av = sizeof(chunk);
    }
    if (av>(playsize-currplay)) {
        av = playsize-currplay;
    }

    if (av>0) {
        r = stream__read( tap_stream, chunk, av);
        if (r<0) {
            // Error reading
            ESP_LOGE(TAG,"Read error");
            state = TAP_IDLE;
            tap_stream = stream__destroy(tap_stream);
            return;
        }
        tzx__chunk(&tape_data.tzx, chunk, r);
    }
}

static void tapeplayer__task(void*data)
{
    struct tapcmd cmd;

    while (1) {
        if (xQueueReceive(tap_evt_queue, &cmd, 50/portTICK_RATE_MS)==pdTRUE) {

            switch (cmd.cmd) {
            case TAP_CMD_PLAY:
                tapeplayer__do_start_play_from_file(cmd.file);
                break;
            case TAP_CMD_PLAYSTREAM:
                tapeplayer__do_start_play_from_stream(cmd.stream, cmd.stream_size, cmd.is_tzx, cmd.initial_delay);
                break;
            case TAP_CMD_STOP:
                tapeplayer__do_stop();
                break;
            case TAP_CMD_RECORD:
                tapeplayer__do_record();
                break;
            }

        } else {
            // No data
            switch (state) {
            case TAP_PLAYING:
                if (tap_stream==NULL) {
                    ESP_LOGW(TAG,"NULL stream! Cannot play!");
                    state = TAP_IDLE;
                } else {
                    //ESP_LOGI(TAG,"TAP tick!");
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
                    fpga__clear_flags(FPGA_FLAG_TAP_ENABLED | FPGA_FLAG_ULAHACK);
                    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET);
                    state = TAP_IDLE;
                }
            default:
                break;
            }
        }
    }
}

bool tapeplayer__isplaying()
{
    return state != TAP_IDLE;
}

void tapeplayer__init()
{
    tap_evt_queue = xQueueCreate(2, sizeof(struct tapcmd));

    xTaskCreate(tapeplayer__task, "tapeplayer_task", TAPEPLAYER_TASK_STACK_SIZE, NULL, TAPEPLAYER_TASK_PRIORITY, NULL);
}
