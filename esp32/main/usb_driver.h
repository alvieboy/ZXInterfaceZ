#ifndef __USBDRIVER_H__
#define __USBDRIVER_H__

struct usb_device;
struct usb_interface;

struct usb_driver {
    int (*probe)(struct usb_device *dev, struct usb_interface *intf);
    void (*disconnect)(struct usb_interface*dev);
};

int usb_driver__probe(struct usb_device *dev, struct usb_interface *intf);

#endif
