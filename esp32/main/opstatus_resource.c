#include "opstatus_resource.h"
#include "fpga.h"
#include <string.h>

void opstatus_resource__set_status(struct opstatus_resource *r, uint8_t value, const char *str)
{
    // TODO: add semaphore
    strcpy( r->l_str, str);
    r->l_status = value;
}


void opstatus_resource__update(struct resource *res)
{
    struct opstatus_resource *sr =(struct opstatus_resource*)res;
    strcpy( sr->str, sr->l_str );
    sr->status = sr->l_status;
}

int opstatus_resource__sendToFifo(struct resource *res)
{
    struct opstatus_resource *sr =(struct opstatus_resource*)res;

    int r = fpga__load_resource_fifo( &sr->status, 1, RESOURCE_DEFAULT_TIMEOUT);
    if (r<0)
        return -1;

    uint8_t len = strlen(sr->str);

    r = fpga__load_resource_fifo( &len, sizeof(len), RESOURCE_DEFAULT_TIMEOUT);
    if (r<0)
        return -1;

    return fpga__load_resource_fifo( (uint8_t*)sr->str, len, RESOURCE_DEFAULT_TIMEOUT);
}

uint8_t opstatus_resource__type(struct resource *res)
{
    return RESOURCE_TYPE_OPSTATUS;
}

uint16_t opstatus_resource__len(struct resource *res)
{
    struct opstatus_resource *sr =(struct opstatus_resource*)res;
    return strlen(sr->str) + 2;
}


