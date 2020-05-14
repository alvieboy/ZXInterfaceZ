#include "int8_resource.h"
#include "fpga.h"

void int8_resource__update(struct resource *res)
{
    struct int8_resource *sr =(struct int8_resource*)res;
    if (sr->valptr!=NULL) {
        sr->latched_val = *(sr->valptr);
    }
}

int int8_resource__sendToFifo(struct resource *res)
{
    struct int8_resource *sr =(struct int8_resource*)res;

    return fpga__load_resource_fifo( (uint8_t*)&sr->latched_val, 1, RESOURCE_DEFAULT_TIMEOUT);
}

uint8_t int8_resource__type(struct resource *res)
{
    return RESOURCE_TYPE_INTEGER;
}

uint16_t int8_resource__len(struct resource *res)
{
    return 1;
}


