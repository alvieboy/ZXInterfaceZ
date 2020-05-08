#ifndef __OPSTATUS_RESOURCE_H__
#define __OPSTATUS_RESOURCE_H__

#include "resource.h"

struct opstatus_resource
{
    struct resource r;
    uint8_t status;
    uint8_t l_status;
    char str[31];
    char l_str[31];
};


void opstatus_resource__update(struct resource *res);
int opstatus_resource__sendToFifo(struct resource *res);
uint8_t opstatus_resource__type(struct resource *res);
uint16_t opstatus_resource__len(struct resource *res);
void opstatus_resource__set_status(struct opstatus_resource *r, uint8_t value, const char *str);

#define OPSTATUS_RESOURCE_DEF { \
    .update = &opstatus_resource__update, \
    .sendToFifo = &opstatus_resource__sendToFifo, \
    .type = &opstatus_resource__type, \
    .len = &opstatus_resource__len \
    }

#endif
