#ifndef __STRING_RESOURCE_H__
#define __STRING_RESOURCE_H__

#include "resource.h"

struct string_resource
{
    struct resource r;
    const char *str;
};


int string_resource__sendToFifo(struct resource *res);
uint8_t string_resource__type(struct resource *res);
uint8_t string_resource__len(struct resource *res);

#define STRING_RESOURCE_DEF { \
    .update = NULL, \
    .sendToFifo = &string_resource__sendToFifo, \
    .type = &string_resource__type, \
    .len = &string_resource__len \
    }

#endif
