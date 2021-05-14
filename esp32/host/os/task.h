#ifndef __OS_TASK_H__
#define __OS_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

typedef pthread_t Task;



int task__create( void(*handler)(void*),
                 const char *name,
                 unsigned stack,
                 void *param,
                 int priority,
                 Task *task);
void task__delete(Task*);
void task__yield();
void task__yield_from_isr();
void task__delay_ms(unsigned ms);
void task__delay(unsigned ticks);
void task__enter_critical();
void task__exit_critical();

Task *task__current_task_handle();

#ifdef __cplusplus
}
#endif

#endif
