#include "string_resource.h"
#include "fpga.h"
#include <string.h>

int string_resource__sendToFifo(struct resource *res)
{
    struct string_resource *sr =(struct string_resource*)res;

    uint8_t len = strlen(sr->str);

    int r = fpga__load_resource_fifo( &len, sizeof(len), RESOURCE_DEFAULT_TIMEOUT);
    if (r<0)
        return -1;
    return fpga__load_resource_fifo( (uint8_t*)sr->str, len, RESOURCE_DEFAULT_TIMEOUT);
}

uint8_t string_resource__type(struct resource *res)
{
    return RESOURCE_TYPE_STRING;
}

uint8_t string_resource__len(struct resource *res)
{
    struct string_resource *sr =(struct string_resource*)res;
    return strlen(sr->str) + 1;
}


