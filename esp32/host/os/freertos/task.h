#include "os/task.h"

typedef void *TaskHandle_t;

#define taskYIELD task__yield
#define vTaskDelete task__delete
#define taskENTER_CRITICAL task__enter_critical
#define taskEXIT_CRITICAL task__exit_critical
#define xTaskCreate task__create
#define portYIELD_FROM_ISR task__yield_from_isr
#define vTaskDelay task__delay
#define xTaskGetCurrentTaskHandle task__current_task_handle

#define vTaskStartScheduler(x)
