#include "usb_device.h"
#include "systemevent.h"
#include <stdlib.h>
#include "log.h"
#include "usbhub.h"
#include "usb_driver.h"
#include "bitmap_allocator.h"
#include "esp_assert.h"

#define TAG "USBDEVICE"

static bitmap32_t usb_id_bitmap;

static struct usb_device_entry *usb_devices = NULL;

static void usbdevice__release(struct object *obj)
{
    struct usb_device *dev = obj_to_usb_device(obj);

    ESP_LOGI(TAG, "Releasing USB device");

    usb_ll__release_channel(dev->ep0_chan);

    if (dev->config_descriptor)
        free(dev->config_descriptor);

    if (dev->vendor)
        free(dev->vendor);
    if (dev->product)
        free(dev->product);
    if (dev->serial)
        free(dev->serial);

}

static const struct object_ops usb_device_object_ops = {
    .release = usbdevice__release
};


const struct usb_device_entry *usbdevice__get_devices()
{
    // THIS IS NOT RACE-FREE!!
    return usb_devices;
}

struct usb_device *usbdevice__new(usb_speed_t speed, struct usb_hub *hub, int port)
{
    struct usb_device *dev = OBJECT_NEW(struct usb_device, &usb_device_object_ops);

    dev->fullspeed = (speed == USB_FULL_SPEED?1:0);

    dev->ep0_chan = usb_ll__alloc_channel(dev->address,
                                          EP_TYPE_CONTROL,
                                          speed == USB_FULL_SPEED ? 64 : 8,
                                          0x00,
                                          speed==USB_FULL_SPEED?1:0,
                                          &dev
                                         );

    dev->hub = hub;
    dev->hub_port = port;

    return dev;
}

void usbdevice__add_device(struct usb_device *dev)
{
    struct usb_device_entry *e = (struct usb_device_entry*)malloc(sizeof(struct usb_device_entry));
    if (e==NULL) {
        return;
    }
    e->dev = usbdevice__get(dev);
    e->next = usb_devices;
    usb_devices = e;

    systemevent__send(SYSTEMEVENT_TYPE_USB, SYSTEMEVENT_USB_DEVICE_CHANGED);

}

struct usb_device *usbdevice__find_by_hub_port(const struct usb_hub *h, int port)
{
    struct usb_device_entry *e = usb_devices;
    while (e) {
        if ((e->dev->hub == h) && (e->dev->hub_port ==port))
            return e->dev;
    }
    return NULL;
}

void usbdevice__disconnect(struct usb_device *dev)
{
    ESP_LOGE(TAG, "Disconnected callback");

    struct usb_device_entry *e = usb_devices;
    struct usb_device_entry *prev = NULL;

    do {
        if (e->dev == dev) {
            if (prev) {
                prev->next = e->next;
            } else {
                usb_devices = e->next;
            }
            usbdevice__put(dev);
            free(e);
            break;
        }
        prev = e;
    } while (e);

    for (int i=0;i< (dev->config_descriptor->bNumInterfaces);i++) {
        ESP_LOGI("USBDEVICE", "Disconnecting interface %d",i);
        usb_driver__disconnect(dev, &dev->interfaces[i]);
    }

}

int usbdevice__allocate_address(void)
{
    return bitmap_allocator__alloc(&usb_id_bitmap);
}

void usbdevice__release_address(int address)
{
    bitmap_allocator__release(&usb_id_bitmap, address);
}

void usbdevice__init()
{
    bitmap_allocator__init(&usb_id_bitmap);
    // Reserve ID 0.
    assert( bitmap_allocator__alloc(&usb_id_bitmap) == 0);
}
