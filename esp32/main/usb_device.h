#ifndef __USBDEVICE_H__
#define __USBDEVICE_H__

#include "esp_attr.h"
#include "usb_ll.h"
#include "usb_defs.h"

struct usb_driver;
struct usb_hub;

struct usb_interface
{
    int8_t numsettings;
    uint8_t claimed;
    uint8_t descriptorlen[2]; // Max 2 alt settings.
    const struct usb_driver *drv;
    void *drvdata; // Driver-specific data
    usb_interface_descriptor_t *descriptors[2]; // Max 2 alt settings.
};

struct usb_device
{
    uint8_t ep0_chan;
    uint8_t ep0_size;
    uint8_t address;
    uint8_t claimed;
    struct usb_hub *hub;
    usb_device_descriptor_t device_descriptor;
    usb_config_descriptor_t config_descriptor_short;
    usb_config_descriptor_t *config_descriptor;
    char *vendor;
    char *product;
    char *serial;
    struct usb_interface interfaces[0];
};

struct usb_device_entry {
    struct usb_device_entry *next;
    struct usb_device *dev;
};

const struct usb_device_entry *usbdevice__get_devices();
void usbdevice__add_device(struct usb_device *dev);


#endif
