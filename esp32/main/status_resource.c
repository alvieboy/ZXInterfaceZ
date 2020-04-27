#include "status_resource.h"
#include "wifi.h"
#include "sdcard.h"
#include "fpga.h"

void status_resource__update(struct resource *res)
{
    struct status_resource *sr =(struct status_resource*)res;

    sr->status = 0;

    if (wifi__issta()) {
        sr->status |= (1<<STATUS_BIT_WIFI_MODE_STA);
    }
    if (wifi__isconnected()) {
        sr->status |= (1<<STATUS_BIT_WIFI_CONNECTED);
    }
    if (wifi__scanning()) {
        sr->status |= (1<<STATUS_BIT_WIFI_SCANNING);
    }
    if (sdcard__isconnected()) {
        sr->status |= (1<<STATUS_BIT_SDCONNECTED);
    }
}

int status_resource__sendToFifo(struct resource *res)
{
    struct status_resource *sr =(struct status_resource*)res;
    return fpga__load_resource_fifo( &sr->status, sizeof(sr->status), RESOURCE_DEFAULT_TIMEOUT);

}

uint8_t status_resource__type(struct resource *res)
{
    return RESOURCE_TYPE_INTEGER;
}

uint16_t status_resource__len(struct resource *res)
{
    return sizeof(uint8_t);
}


