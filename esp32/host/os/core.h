#ifndef __OS_CORE_H__
#define __OS_CORE_H__

#include <sys/time.h>
#include <time.h>

#define OS_MAX_DELAY 0xFFFFFFFF
#define OS_TRUE (0)
#define OS_PASS (0)
#define OS_FALSE (-1)

#define OS_TICK_RATE_MS (1)

#define NSEC_PER_SEC 1000000000

static inline void timespec_normalise(struct timespec *ts)
{
    while(ts->tv_nsec >= NSEC_PER_SEC)
    {
        ++(ts->tv_sec);
        ts->tv_nsec -= NSEC_PER_SEC;
    }
}

static inline void os__timeout_to_timespec(unsigned timeout, struct timespec *t)
{
    clock_gettime(CLOCK_REALTIME, t);

    // timeout is in units of 10ms (ESP32 scheduler is 100Hz).

    // Example:
    // 150 = 1.5s = 1500ms
    // = 1s,
    // 50
    // 500 (ms) x10
    // 500000 (us)   x1000
    // 500000000 (ns)   x1000

    
    t->tv_sec += timeout / 100;
    t->tv_nsec += (timeout % 100)*10000000;
    timespec_normalise(t);
}

#endif
