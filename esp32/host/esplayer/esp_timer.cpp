#include "esp_timer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

static bool initialized = false;

static SemaphoreHandle_t timer_sem;

struct esp_timer {
    bool active;
    esp_timer_cb_t callback;
    void *arg;
    unsigned delta;
    unsigned cnt;
};

typedef std::vector<struct esp_timer *> timerlist_t;

static timerlist_t timers;

static void esp_timer_lock()
{
    if (xSemaphoreTake( timer_sem,  portMAX_DELAY )!= pdTRUE) {
        abort();
    }
}

static void esp_timer_unlock()
{
    xSemaphoreGive(timer_sem);
}

esp_err_t esp_timer_delete(struct esp_timer *timer)
{
    esp_timer_lock();
    timerlist_t::iterator i;
    i = std::find(timers.begin(), timers.end(), timer);
    if (i!=timers.end())
        timers.erase(i);
    esp_timer_unlock();
    return 0;
}

extern "C" void esp_timer_task(void*)
{
    while (1) {
        esp_timer_lock();
        for (auto i: timers) {
            if (i->active) {
                if (i->cnt==0) {
                    i->cnt = i->delta;
                    esp_timer_unlock();
                    i->callback(i->arg);
                    esp_timer_lock();
                } else {
                    i->cnt--;
                }
            }
        }
        esp_timer_unlock();
        vTaskDelay(20 / portTICK_RATE_MS);
    }
}

static void esp_timer_init_pvt()
{
    if (initialized)
        return;

    timer_sem = xSemaphoreCreateMutex();

    if (xTaskCreate(&esp_timer_task, "wsys_task", 4096, NULL, 6, NULL)!=pdPASS) {
        
    }

    initialized = true;
}


extern "C" esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period)
{
    esp_timer_lock();
    timer->delta = period / 20000;
    timer->cnt = timer->delta;
    timer->active = true;
    esp_timer_unlock();

    return 0;
}

extern "C" esp_err_t esp_timer_create(const esp_timer_create_args_t* create_args,
                           esp_timer_handle_t* out_handle)
{
    esp_timer_init_pvt();

    esp_timer *h = new esp_timer();
    h->active = false;
    h->cnt = 0;
    h->delta = 0;
    h->arg = create_args->arg;
    h->callback = create_args->callback;
    esp_timer_lock();
    timers.push_back(h);
    esp_timer_unlock();

    *out_handle = h;

    return 0;
}

extern "C" esp_err_t esp_timer_init(void)
{
    esp_timer_init_pvt();
    return 0;
}



