#include "os/queue.h"
#include <pthread.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

struct queue_impl
{
    pthread_cond_t queue_not_full, queue_not_empty;
    pthread_mutex_t queue_lock;
    void **data;
    unsigned num_items;
    unsigned each_size;
    int head;
    int tail;
};

Queue queue__create(unsigned num_items, unsigned each_size)
{
    Queue q = (Queue)malloc(sizeof(struct queue_impl));

    num_items++; //

    pthread_cond_init(&q->queue_not_empty, NULL);
    pthread_cond_init(&q->queue_not_full, NULL);
    pthread_mutex_init(&q->queue_lock, NULL);

    q->data = malloc( num_items * sizeof(void*) );
    for (unsigned i=0;i<num_items;i++) {
        q->data[i] = malloc(each_size);
    }
    q->num_items = num_items;
    q->each_size = each_size;
    q->head = 0;
    q->tail = 0;

    return q;
}

void queue__delete(Queue q)
{
    pthread_mutex_lock(&q->queue_lock);

    for (unsigned i=0;i<q->num_items;i++) {
        free(q->data[i]);
    }
    free(q->data);
    q->data = NULL;

    pthread_cond_destroy(&q->queue_not_empty);
    pthread_cond_destroy(&q->queue_not_full);

    pthread_mutex_unlock(&q->queue_lock);
    pthread_mutex_destroy(&q->queue_lock);
    free(q);
}

int queue__send(Queue q, void *data, unsigned timeout)
{
    struct timespec t;
    volatile Queue vq = (volatile Queue)q;
    int r;

    if (q==NULL)
        return -1;

    os__timeout_to_timespec(timeout, &t);

    if (timeout!=OS_MAX_DELAY) {
        r = pthread_mutex_timedlock(&q->queue_lock, &t);
    } else {
        r = pthread_mutex_lock(&q->queue_lock);
    }

    if (r<0)
        return r;

    do {

        int newhead = vq->head+1;
        if (newhead>=(int)vq->num_items)
            newhead=0;

        if (newhead==vq->tail) {
            // Full.
            if (timeout==OS_MAX_DELAY) {
                pthread_cond_wait(&q->queue_not_full, &vq->queue_lock);
            } else {
                if (pthread_cond_timedwait(&q->queue_not_full, &vq->queue_lock, &t)!=0) {
                    pthread_mutex_unlock(&q->queue_lock);
                    return OS_FALSE;
                }
            }

        } else {
            memcpy( vq->data[vq->head], data, vq->each_size );
            vq->head = newhead;
            bool broadcast = false;
            if ((newhead - vq->tail)==1 ||
                (newhead - vq->tail)==-1) {
                broadcast = true;
            }
            pthread_cond_broadcast( &vq->queue_not_empty );
            pthread_mutex_unlock (&vq->queue_lock );
            return OS_TRUE;
        }
    } while (1);
    pthread_mutex_unlock (&vq->queue_lock );

    return -1;
}

#include <sys/errno.h>

int queue__receive(Queue q, void *data, unsigned timeout)
{
    struct timespec t;
    volatile Queue vq = (volatile Queue)q;
    int r;

    if (q==NULL)
        return -1;

    os__timeout_to_timespec(timeout, &t);

    if (timeout!=OS_MAX_DELAY) {
        r = pthread_mutex_timedlock(&q->queue_lock, &t);
    } else {
        r = pthread_mutex_lock(&q->queue_lock);
    }

    if (r<0)
        return r;

    do {
        if (vq->head == vq->tail) {
            // Empty.
            //printf("Queue empty %d\n", timeout);
            if (timeout==0) {
                return -1;
            }
            if (timeout==OS_MAX_DELAY){
                r = pthread_cond_wait(&q->queue_not_empty, &vq->queue_lock);
                if (r<0)
                    abort();
            } else {
                int s = (int)pthread_self();
                //printf("%d: Timed wait %d.%ld\n", s, t.tv_sec, t.tv_nsec);
                int r =pthread_cond_timedwait(&q->queue_not_empty, &vq->queue_lock, &t);
                if (r!=0) {
                  //  printf("%d: Wait end %d errno=%d\n", s, r, errno);
                    pthread_mutex_unlock (&vq->queue_lock );
                    return OS_FALSE;
                } else {
                    //printf("%d: Wait OK %d\n", s, r);
                    if (vq->head == vq->tail) {
                        abort();
                    }
                }
            }

        } else {
            memcpy( data, vq->data[vq->tail], vq->each_size );
            vq->tail++;

            if (vq->tail >= vq->num_items)
                vq->tail = 0;

            pthread_cond_broadcast( &vq->queue_not_full );
            pthread_mutex_unlock (&vq->queue_lock );

            return OS_TRUE;
        }
    } while (1);
    pthread_mutex_unlock (&vq->queue_lock );
    return -1;
}

int queue__send_from_isr(Queue q, void *data, int*wakeup)
{
    queue__send(q,data,OS_MAX_DELAY);
    if (wakeup)
        *wakeup=1;
    return OS_TRUE;
}

int queue__send_to_front(Queue q, void *data, unsigned timeout)
{
    struct timespec t;
    volatile Queue vq = (volatile Queue)q;
    int r;

    if (q==NULL)
        return -1;

    os__timeout_to_timespec(timeout, &t);

    if (timeout!=OS_MAX_DELAY) {
        r = pthread_mutex_timedlock(&q->queue_lock, &t);
    } else {
        r = pthread_mutex_lock(&q->queue_lock);
    }

    if (r<0)
        return r;

    do {

        int newtail = vq->tail-1;
        if (newtail<0)
            newtail = vq->num_items;

        if (newtail==vq->tail) {
            // Full.
            if (timeout==OS_MAX_DELAY){
                pthread_cond_wait(&q->queue_not_empty, &vq->queue_lock);
            } else {
                if (pthread_cond_timedwait(&q->queue_not_empty, &vq->queue_lock, &t)!=0) {

                    pthread_mutex_unlock (&vq->queue_lock );
                    return OS_FALSE;
                }
            }

        } else {
            memcpy( vq->data[newtail], data, vq->each_size );
            vq->tail = newtail;
            pthread_cond_broadcast( &vq->queue_not_empty );
            pthread_mutex_unlock (&vq->queue_lock );
            return OS_TRUE;
        }
    } while (1);

    pthread_mutex_unlock (&vq->queue_lock );

    return -1;
}


