#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "os/core.h"

typedef xQueueHandle Queue;
#define QUEUE_INIT NULL

static inline Queue queue__create(unsigned num_items, unsigned each_size)
{
    return xQueueCreate(num_items, each_size);
}

static inline int queue__send(Queue q, const void *data,
                                unsigned timeout)
{
    return xQueueSend(q,data,timeout);
}

static inline int queue__send_to_front(Queue q, const void *data,
                                       unsigned timeout)
{
    return xQueueSendToFront(q,data,timeout);
}

static inline int queue__send_from_isr(Queue q, const void *data, int *wakeup)
{
    return xQueueSendFromISR(q, data, wakeup);
}

static inline int queue__receive(Queue q, void *data,
                                 unsigned timeout)
{
    return xQueueReceive(q,data,timeout);
}

static inline void queue__delete(Queue q)
{
    vQueueDelete(q);
}

#endif
