#include "event_task.h"
#include "os/task.h"
#include "os/queue.h"
#include "os/semaphore.h"
#include "esp_event.h"
#include "list.h"
#include <vector>

static Queue event_queue = NULL;
static Semaphore mutex;

//    xQueueSend(scan_queue, &val, 1000);

struct hentry {
    esp_event_base_t event_base;
    int32_t event_id;
    esp_event_handler_t event_handler;
    void *event_handler_arg;
};

static std::vector<hentry> handlers;

void event_task(void *p)
{
    struct esp_event_wrap w;
    while (1)
    {
        if (xQueueReceive(event_queue, &w, portMAX_DELAY)==pdTRUE) {
            std::vector<hentry>::iterator it;

            xSemaphoreTake(mutex, portMAX_DELAY);
            for (it = handlers.begin(); it!=handlers.end();) {
                if (it->event_base == w.event_base) {
                    it->event_handler(it->event_handler_arg,
                                      w.event_base,
                                      w.event_id,
                                      w.event_data
                                     );
                    break;
                } else {
                    it++;
                }
            }
            xSemaphoreGive(mutex);
        }
    }
}

void event_task_init(void)
{
    event_queue = xQueueCreate(4, sizeof(struct esp_event_wrap));
    mutex = xSemaphoreCreateBinary();
    xSemaphoreGive(mutex);
}


esp_err_t esp_event_send_internal(esp_event_base_t event_base,
                                  int32_t event_id,
                                  void* event_data,
                                  size_t event_data_size,
                                  TickType_t ticks_to_wait)
{
    struct esp_event_wrap w;
    w.event_base = event_base;
    w.event_id = event_id;
    w.event_data = event_data;
    w.event_data_size = event_data_size;
    xQueueSend(event_queue, &w, ticks_to_wait);

}

esp_err_t esp_event_handler_register(esp_event_base_t event_base,
                                     int32_t event_id,
                                     esp_event_handler_t event_handler,
                                     void *event_handler_arg)
{
    struct hentry h;
    h.event_base = event_base;
    h.event_id = event_id;
    h.event_handler = event_handler;
    h.event_handler_arg = event_handler_arg;

    xSemaphoreTake(mutex, portMAX_DELAY);
    handlers.push_back(h);
    xSemaphoreGive(mutex);
    return 0;
}

esp_err_t esp_event_handler_unregister(esp_event_base_t event_base,
                                       int32_t event_id,
                                       esp_event_handler_t event_handler)
{
    std::vector<hentry>::iterator it;

    semaphore__take(mutex, portMAX_DELAY);
    for (it = handlers.begin(); it!=handlers.end();it++) {
        if ((it->event_handler == event_handler)
            && (it->event_id == event_id)
            && (it->event_base  == event_base))
        {
            handlers.erase(it);
            break;
        }
    }
    xSemaphoreGive(mutex);
    return 0;
}

esp_err_t esp_event_loop_create_default()
{
    return 0;
}
