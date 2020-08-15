#include "buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "gpio.h"
#include "defs.h"
static uint8_t button_counters[2];

#define DEBOUNCE_MAX 10
static uint8_t debounce[2];

#define PRESS_SHORT 8
#define PRESS_LONG 254
#define PRESS_MAX  255

static xQueueHandle button_event_queue;
static TaskHandle_t button_task;

static inline void button__check(int index, uint32_t gpio_num)
{
    // Debounce button
    button_event_t event;
    event.button = index;
    event.type = BUTTON_IDLE;

    // ALL CALLS need to be on IRAM, because we might be accessing the flash.

    int v = gpio__read(gpio_num);
    if (v==0) {
        if (debounce[index]>0)
            debounce[index]--;
    } else {
        if (debounce[index]<DEBOUNCE_MAX)
            debounce[index]++;
    }
    if (debounce[index]==0) {
        // Button fully down
        if (button_counters[index]<PRESS_MAX)
            button_counters[index]++;

        if (button_counters[index] == PRESS_LONG) {
            // Emit short press
            event.type = BUTTON_LONG_PRESSED;
        }

        if (button_counters[index] == PRESS_SHORT) {
            // Emit long press
            event.type = BUTTON_PRESSED;
        }
    }

    if (debounce[index]==DEBOUNCE_MAX) {
        // Button fully up
        if (button_counters[index]!=0) {
            // Release event
            if (button_counters[index]>=PRESS_LONG) {
                event.type = BUTTON_LONG_RELEASED;
            } else {
                event.type = BUTTON_RELEASED;
            }
            button_counters[index]=0;
        }
    }
    if (event.type!=BUTTON_IDLE) {
        printf("Button event\n");
        xQueueSend(button_event_queue, &event, NULL);
    }
}


void button_scan_task(void *data)
{
    ESP_LOGI("Button","Scan task starting");
    while (1) {
        button__check(0, PIN_NUM_SWITCH);
        button__check(1, PIN_NUM_IO0);
        vTaskDelay(10 / portTICK_RATE_MS);
    };
}

void buttons__init(void)
{
    button_event_queue = xQueueCreate(4, sizeof(button_event_t));

    debounce[0] = 3;
    debounce[1] = 3;
    button_counters[0] = 0;
    button_counters[1] = 0;
    ESP_LOGI("Buttons", "Starting button scan task");
    xTaskCreate(button_scan_task, "button_scan", 4096, NULL, 6, &button_task);
}

int buttons__getevent(button_event_t *event, bool block)
{
    if (!xQueueReceive(button_event_queue, event, block? portMAX_DELAY:0))
        return -1;
    return 0;
}


