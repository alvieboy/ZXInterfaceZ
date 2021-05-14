#include "os/semaphore.h"

typedef Semaphore SemaphoreHandle_t;

#define xSemaphoreTake semaphore__take
#define xSemaphoreGive semaphore__give
#define xSemaphoreCreateBinary semaphore__create_binary
#define xSemaphoreCreateMutex semaphore__create_mutex
