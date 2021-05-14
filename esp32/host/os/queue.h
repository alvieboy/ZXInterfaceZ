#ifndef __OS_QUEUE_H__
#define __OS_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "os/core.h"



struct queue_impl;
typedef struct queue_impl * Queue;

#define QUEUE_INIT NULL

Queue queue__create(unsigned num_items, unsigned each_size);
int queue__send(Queue q, void *data, unsigned timeout);
int queue__send_to_front(Queue q, void *data, unsigned timeout);
int queue__send_from_isr(Queue q, void *data, int*wakeup);
int queue__receive(Queue q, void *data, unsigned timeout);
void queue__delete(Queue);

#ifdef __cplusplus
}
#endif

#endif


