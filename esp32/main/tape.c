#include "tape.h"
#include "log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "save.h"
#include "tapeplayer.h"

#define TAG "TAPE"
#define MIC_TIMEOUT 15 /* 15 seconds */
#define TAP_TIMEOUT 15 /* 15 seconds */

static SemaphoreHandle_t tape_sem;
static esp_timer_handle_t tape_timer;
static tapemode_t tapemode;
static bool ignore_mic = false;
static uint8_t tap_timer;

static void tape__lock()
{
    if (xSemaphoreTake( tape_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return;
    }
}

static void tape__unlock()
{
    xSemaphoreGive(tape_sem);
}


static void tape__timer_elapsed();

void tape__init(void)
{
    tape_sem = xSemaphoreCreateMutex();
    if (tape_sem==NULL) {
        ESP_ERROR_CHECK(-1);
    }

    const esp_timer_create_args_t timer_args = {
        .callback = &tape__timer_elapsed,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "tapetimer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &tape_timer));

    esp_timer_start_periodic(tape_timer, 1000000);
}

bool tape__is_tape_loaded(void)
{
    return tapemode != TAPE_NO_TAPE;
}

bool tape__is_tape_playing(void)
{
    bool r = false;
    switch(tapemode) {
    case TAPE_PHYSICAL_LOAD: /* Fall-through */
    case TAPE_TAP_LOAD:      /* Fall-through */
    case TAPE_TZX_LOAD:
        r = true;
        break;
    default:
        break;
    }
    return r;
}

bool tape__is_tape_saving(void)
{
    bool r = false;
    switch(tapemode) {
    case TAPE_PHYSICAL_SAVE: /* Fall-through */
    case TAPE_TAP_SAVE:      /* Fall-through */
    case TAPE_TZX_SAVE:
        r = true;
        break;
    default:
        break;
    }
    return r;
}

tapemode_t tape__get_tape_mode(void)
{
    return tapemode;
}

int tape__enter_tape_mode(tapemode_t newmode)
{
    // Not all combinations are valid.
    tape__lock();
    switch (newmode) {
    case TAPE_PHYSICAL_LOAD: /* Fall-through */
    case TAPE_PHYSICAL_SAVE:
        ignore_mic = true; // Ignore MIC for one sample.
        break;
    default:
        break;
    }
    tape__unlock();
    return 0;
}


void tape__notify_load(void)
{
    tape__lock();
    tap_timer = 0;
    tape__unlock();
}

void tape__notify_save(void)
{
    tape__lock();
    tap_timer = 0;
    tape__unlock();
}

void tape__notify_mic_idle(uint8_t value)
{
    tape__lock();
    switch (tapemode) {
    case TAPE_PHYSICAL_LOAD: /* Fall-through */
    case TAPE_PHYSICAL_SAVE:
        if (!ignore_mic) {
            if (value > MIC_TIMEOUT) {
                ESP_LOGI(TAG,"MIC timeout, disconnecting physical tape");
                tapemode = TAPE_NO_TAPE;
                break;
            }
        }
        ignore_mic = false;
        break;
    default:
        break;
    }
    tape__unlock();
}

static void tape__timer_elapsed()
{
    bool issave = false;
    tape__lock();
    switch (tapemode) {
    case TAPE_TAP_SAVE:
        issave = true; /* Fall-through */
    case TAPE_TAP_LOAD:
        tap_timer++;
        if (tap_timer>TAP_TIMEOUT) {
            // Disconnect
            tapemode = TAPE_NO_TAPE;
            if (issave) {
                save__stop_tape();
            } else {
                tapeplayer__stop();
            }
        }
        break;
    default:
        break;
    }
    tape__unlock();
}

