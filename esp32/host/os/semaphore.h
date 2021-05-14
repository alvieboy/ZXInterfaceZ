#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <pthread.h>
#include "os/core.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef pthread_mutex_t *Semaphore;

int semaphore__take(Semaphore s, unsigned timeout);

static inline int semaphore__give(Semaphore s)
{
    return pthread_mutex_unlock(s)==0?OS_TRUE:-1;
}

Semaphore semaphore__create_mutex();
Semaphore semaphore__create_binary();

#ifdef __cplusplus
}
#endif

#endif
