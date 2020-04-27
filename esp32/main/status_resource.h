#ifndef __STATUS_RESOURCE_H__
#define __STATUS_RESOURCE_H__

#include "resource.h"

#define STATUS_BIT_WIFI_MODE_STA 0
#define STATUS_BIT_WIFI_CONNECTED 1
#define STATUS_BIT_WIFI_SCANNING 2
#define STATUS_BIT_SDCONNECTED 3

struct status_resource
{
    struct resource r;
    uint8_t status;
};

void status_resource__update(struct resource *res);
int status_resource__sendToFifo(struct resource *res);
uint8_t status_resource__type(struct resource *res);
uint16_t status_resource__len(struct resource *res);

#define STATUS_RESOURCE_DEF { \
    .update = &status_resource__update, \
    .sendToFifo = &status_resource__sendToFifo, \
    .type = &status_resource__type, \
    .len = &status_resource__len \
    }

#endif
