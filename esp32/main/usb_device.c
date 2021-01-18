#include "usb_device.h"
#include "systemevent.h"
#include <stdlib.h>
#include "log.h"
#include "usbhub.h"
#include "usb_driver.h"

static struct usb_device_entry *usb_devices = NULL;

const struct usb_device_entry *usbdevice__get_devices()
{
    // THIS IS NOT RACE-FREE!!
    return usb_devices;
}

void usbdevice__add_device(struct usb_device *dev)
{
    struct usb_device_entry *e = (struct usb_device_entry*)malloc(sizeof(struct usb_device_entry));
    if (e==NULL) {
        return;
    }
    e->dev = dev;
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
    ESP_LOGE("USBDEVICE", "Disconnected callback");

    struct usb_device_entry *e = usb_devices;
    struct usb_device_entry *prev = NULL;

    do {
        if (e->dev == dev) {
            if (prev) {
                prev->next = e->next;
            } else {
                usb_devices = e->next;
            }
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
