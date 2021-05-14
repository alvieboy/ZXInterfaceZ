#include "os/semaphore.h"
#include "os/core.h"
#include <malloc.h>

int semaphore__take(Semaphore s, unsigned timeout)
{
    struct timespec t;
    int r;

    if (timeout!=OS_MAX_DELAY) {
        t.tv_sec = timeout / 100;
        t.tv_nsec = (timeout % 100)*1000000;
        r = pthread_mutex_timedlock(s, &t);
    } else {
        r = pthread_mutex_lock(s);
    }
    if (r==0)
        return OS_TRUE;
    return r;
}

Semaphore semaphore__create_mutex()
{
    pthread_mutex_t *mutex = malloc(sizeof (pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    return mutex;
}

Semaphore semaphore__create_binary()
{
    pthread_mutex_t *mutex = malloc(sizeof (pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    return mutex;
}


