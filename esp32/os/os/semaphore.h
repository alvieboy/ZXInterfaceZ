#ifndef __OS_SEMAPHORE_H__
#define __OS_SEMAPHORE_H__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef xSemaphoreHandle Semaphore;

#define SEMAPHORE_INIT NULL

static inline Semaphore semaphore__create_mutex()
{
    return xSemaphoreCreateMutex();
}

static inline Semaphore semaphore__create_binary()
{
    return xSemaphoreCreateBinary();
}

static inline int semaphore__take(Semaphore s, unsigned timeout)
{
    return xSemaphoreTake( s,  timeout );
}

static inline int semaphore__give(Semaphore s)
{
    return xSemaphoreGive( s );
}

#endif
