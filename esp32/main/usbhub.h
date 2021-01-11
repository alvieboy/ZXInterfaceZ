#ifndef __USBHUB_H__
#define __USBHUB_H__

#include <inttypes.h>
#include "usb_defs.h"

struct usb_hub
{
    uint8_t usb_address;
    void (*reset)(void);
};

struct usb_device;


int usbhub__port_reset(struct usb_hub *, usb_speed_t speed);
void usbhub__device_disconnect(struct usb_device *dev);

#endif
