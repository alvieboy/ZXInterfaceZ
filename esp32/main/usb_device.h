#ifndef __USBDEVICE_H__
#define __USBDEVICE_H__

#include "esp_attr.h"
#include "usb_ll.h"
#include "usb_defs.h"
#include <stdbool.h>
#include <stdlib.h>
#include "object.h"

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
    struct object obj;
    uint8_t ep0_chan;
    //uint8_t ep0_size;
    uint8_t address;
    uint8_t claimed:1;
    uint8_t fullspeed:1;
    uint8_t has_siblings:1;
    struct usb_hub *hub;
    uint8_t hub_port;
    struct usb_hub *self_hub;
    union {
        uint8_t device_descriptor_b[18];
        usb_device_descriptor_t device_descriptor;
    };
    union {
        uint8_t config_descriptor_short_b[9];
        usb_config_descriptor_t config_descriptor_short;
    };
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

static inline struct usb_device *obj_to_usb_device(struct object *o)
{
    return (struct usb_device *)o;
}

struct usb_device *usbdevice__new(usb_speed_t speed, struct usb_hub *hub, int port);
static inline void usbdevice__put(struct usb_device *dev)
{
    object__put(&dev->obj);
}

static inline struct usb_device *usbdevice__get(struct usb_device *dev)
{
    return (struct usb_device*)object__get(&dev->obj);
}


void usbdevice__init(void);
int usbdevice__allocate_address(void);
void usbdevice__release_address(int address);

const struct usb_device_entry *usbdevice__get_devices();
void usbdevice__add_device(struct usb_device *dev);


static inline bool usbdevice__ishub(struct usb_device *dev) {
    return dev->self_hub!=NULL;
}
static inline struct usb_hub *usbdevice__to_hub(struct usb_device *dev) {
    return dev->self_hub;
}
struct usb_device *usbdevice__find_by_hub_port(const struct usb_hub *h, int port);
void usbdevice__disconnect(struct usb_device *dev);


#endif
