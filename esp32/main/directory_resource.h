#ifndef __DIRECTORY_RESOURCE_H__
#define __DIRECTORY_RESOURCE_H__

#include "resource.h"

struct directory_resource
{
    struct resource r;
    int alloc_size;
    unsigned char *buffer;
    uint8_t entries;
    uint8_t filter;
};


void directory_resource__update(struct resource *res);
int directory_resource__sendToFifo(struct resource *res);
uint8_t directory_resource__type(struct resource *res);
uint16_t directory_resource__len(struct resource *res);

void directory_resource__set_filter(struct directory_resource *r, uint8_t filter);

#define DIRECTORY_RESOURCE_DEF { \
    .update = &directory_resource__update, \
    .sendToFifo = &directory_resource__sendToFifo, \
    .type = &directory_resource__type, \
    .len = &directory_resource__len \
    }

#endif
