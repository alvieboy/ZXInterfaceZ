#ifndef __USBHUB_H__
#define __USBHUB_H__

#include <inttypes.h>
#include "usb_defs.h"

struct usb_hub
{
    int (*init)(struct usb_hub *);
    int (*reset)(struct usb_hub *,int port);
    int (*get_ports)(struct usb_hub *);
    int (*set_power)(struct usb_hub *, int port, int power);
    int (*disconnect)(struct usb_hub *);
};

struct usb_device;


void usbhub__new(struct usb_hub *h);

int usbhub__port_reset(struct usb_hub *, int port, usb_speed_t speed);

void usbhub__port_disconnect(struct usb_hub *hub, int port);

int usbhub__overcurrent(struct usb_hub *, int port);

#endif
