#include "aplist_resource.h"
#include "fpga.h"
#include <string.h>

void aplist_resource__clear(struct aplist_resource*ar)
{
    if (ar->buf) {
        free(ar->buf);
        ar->buf = NULL;
    }
    ar->buflen = 0;
    ar->bufpos = 0;
}

void aplist_resource__resize(struct aplist_resource*ar, uint16_t len)
{
    aplist_resource__clear(ar);
    ar->buf = malloc(len);
    ar->buflen = len;
    ar->bufpos = 0;
}

int aplist_resource__setnumaps(struct aplist_resource*ar, uint8_t num)
{
    if (ar->bufpos!=0)
        return -1;
    ar->buf[ar->bufpos++] = num;
    return 0;
}

int aplist_resource__addap(struct aplist_resource*ar, uint8_t flags, const char *ssid, uint8_t ssidlen)
{
    uint16_t remain = ar->buflen - ar->bufpos;
    if (remain < (ssidlen+1)) {
        return -1;
    }
    ar->buf[ ar->bufpos++ ] = ssidlen;
    // Copy SSID
    memcpy( &ar->buf[ar->bufpos], ssid, ssidlen );
    ar->bufpos += ssidlen;
    return 0;
}

uint8_t aplist_resource__type(struct resource*r)
{
    return RESOURCE_TYPE_APLIST;
}

uint8_t aplist_resource__len(struct resource*r)
{
    struct aplist_resource *ar = (struct aplist_resource*)r;
    return ar->bufpos;
}

int aplist_resource__sendToFifo(struct resource *r)
{
    struct aplist_resource *ar = (struct aplist_resource*)r;
    return fpga__load_resource_fifo(ar->buf, ar->bufpos, RESOURCE_DEFAULT_TIMEOUT);
}

