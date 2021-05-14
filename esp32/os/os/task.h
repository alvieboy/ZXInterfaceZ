#ifndef __OS_TASK_H__
#define __OS_TASK_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef TaskHandle_t Task;


static inline int task__create( void(*handler)(void*),
                               const char *name,
                               unsigned stack,
                               void *param,
                               int priority,
                               Task *task)
{
    return xTaskCreate(handler, name, stack, param, priority, task);
}

static inline void task__delete(Task *task)
{
    vTaskDelete(task);
}

static inline void task__yield()
{
    portYIELD();
}

static inline void task__yield_from_isr()
{
    portYIELD_FROM_ISR();
}

static inline void task__delay(unsigned tick)
{
    vTaskDelay(tick);
}

static inline void task__delay_ms(unsigned ms)
{
    vTaskDelay(ms/portTICK_RATE_MS);
}

#endif
