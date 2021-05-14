#include "os/task.h"
#include "os/core.h"
#include <pthread.h>

int task__create( void(*handler)(void*),
                 const char *name,
                 unsigned stack,
                 void *param,
                 int priority,
                 Task *task)
{
    pthread_attr_t attr;
    pthread_t thr;

    pthread_attr_init(&attr);
    // TBD: properly wrap handler due to void* return
    if (pthread_create(&thr, &attr, handler, param)<0)
        return -1;
    if (task)
        *task = thr;
    pthread_setname_np(thr, name);
    return OS_TRUE;
}

void task__yield()
{
    pthread_yield();
}
void task__yield_from_isr()
{
    pthread_yield();
}

void task__delay_ms(unsigned ms)
{
    usleep(ms*1000);
}

void task__delay(unsigned ticks)
{
    usleep(ticks*10000);
}

void task__enter_critical()
{
}
void task__exit_critical()
{
}

void task__delete(Task *t)
{
    if (t==NULL) {
        pthread_exit(NULL);
    }
    pthread_cancel(t);
}

Task *task__current_task_handle()
{
    return pthread_self();
}

