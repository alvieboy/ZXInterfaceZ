#include "usb_device.h"
#include "systemevent.h"
#include <stdlib.h>
#include "log.h"
#include "usbhub.h"

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

void usb_ll__disconnected_callback()
{
    ESP_LOGE("USB", "Disconnected callback");

    while (usb_devices) {
        // Disconnect this device
        usbhub__device_disconnect(usb_devices->dev);
        struct usb_device_entry *next = usb_devices->next;
        free(usb_devices);
        usb_devices = next;
    }
}
