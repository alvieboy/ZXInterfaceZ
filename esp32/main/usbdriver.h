#ifndef __USBDRIVER_H__
#define __USBDRIVER_H__

struct usb_device;

int usbdriver__probe(struct usb_device *dev);

#endif
