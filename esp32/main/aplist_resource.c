#include "aplist_resource.h"
#include "fpga.h"
#include <string.h>
#include "esp_wifi.h"
#include "wifi.h"

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

uint16_t aplist_resource__len(struct resource*r)
{
    struct aplist_resource *ar = (struct aplist_resource*)r;
    return ar->bufpos;
}

int aplist_resource__sendToFifo(struct resource *r)
{
    struct aplist_resource *ar = (struct aplist_resource*)r;
    return fpga__load_resource_fifo(ar->buf, ar->bufpos, RESOURCE_DEFAULT_TIMEOUT);
}

#if 0

static void aplist_resource__wifi_scan_reset(void *a)
{
    struct aplist_resource *ar = (struct aplist_resource*)a;
    ar->needlen = 0;
}

static void aplist_resource__wifi_apcount(void *a, uint8_t apcount, size_t ssidlensum)
{
    struct aplist_resource *ar = (struct aplist_resource*)a;

    ar->needlen = 1 + ssidlensum + (apcount * 2); // SSIDlen + flags

    aplist_resource__setnumaps(a, apcount );
}

static void aplist_resource__wifi_ap(void *a, uint8_t auth, uint8_t channel, int8_t rssi, const char *ssid, size_t ssidlen)
{
    uint8_t flags = 0;
    switch ( auth ) {
    case WIFI_AUTH_OPEN:
    case WIFI_AUTH_WPA_PSK:
    case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:
            flags=1;
            break;
        default:
            break;
        }
    aplist_resource__addap(a, flags, ssid, ssidlen);
}

static void aplist_resource__wifi_finish(void *a)
{
}

static const wifi_scan_parser_t scan_parser =  {
    .reset = &aplist_resource__wifi_scan_reset,
    .apcount = &aplist_resource__wifi_apcount,
    .ap = &aplist_resource__wifi_ap,
    .finish = &aplist_resource__wifi_finish
};


int aplist_resource__request_wifi_scan(struct aplist_resource *r)
{
    return wifi__scan(&scan_parser, r);
}

#endif