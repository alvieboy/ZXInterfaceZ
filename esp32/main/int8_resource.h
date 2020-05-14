#ifndef __INT8_RESOURCE_H__
#define __INT8_RESOURCE_H__

#include "resource.h"

struct int8_resource
{
    struct resource r;
    int8_t *valptr;
    int8_t latched_val;
};


int int8_resource__sendToFifo(struct resource *res);
uint8_t int8_resource__type(struct resource *res);
void int8_resource__update(struct resource *res);
uint16_t int8_resource__len(struct resource *res);

#define INT8_RESOURCE_DEF { \
    .update = int8_resource__update, \
    .sendToFifo = &int8_resource__sendToFifo, \
    .type = &int8_resource__type, \
    .len = &int8_resource__len \
    }

#endif
