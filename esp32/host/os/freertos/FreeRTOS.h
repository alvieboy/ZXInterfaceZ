#include "os/core.h"
#include "FreeRTOSConfig.h"

typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef long int TickType_t;

typedef void(*TaskFunction_t)(void*);

#define portMAX_DELAY OS_MAX_DELAY
#define portTICK_RATE_MS (10)
#define pdTRUE (0)
#define pdPASS (0)
