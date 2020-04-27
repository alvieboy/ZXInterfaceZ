#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "esp_log.h"
#include "defs.h"

#define RESOURCE_DEFAULT_TIMEOUT 5000


#define RESOURCE_TYPE_INVALID (0xff)
#define RESOURCE_TYPE_INTEGER (0x00)
#define RESOURCE_TYPE_STRING (0x01)
#define RESOURCE_TYPE_BITMAP (0x02)
#define RESOURCE_TYPE_DIRECTORYLIST (0x03)
#define RESOURCE_TYPE_APLIST (0x04)

struct resource
{
    void (*update)(struct resource *);
    uint8_t (*type)(struct resource *);
    uint16_t (*len)(struct resource *);
    int (*sendToFifo)(struct resource *);
};

void resource__init(void);
struct resource *resource__find(uint8_t id);
void resource__register(uint8_t id, struct resource *res);
int resource__sendtofifo(struct resource *res);



#endif
