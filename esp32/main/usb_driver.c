#include "usb_driver.h"
#include "usb_defs.h"
#include "usbh.h"
#include <stdlib.h>
#include "usb_descriptor.h"
#include "log.h"

extern const struct usb_driver *usb_driver_list[];
extern const unsigned usb_driver_list_count;

int usb_driver__probe(struct usb_device *dev, struct usb_interface *intf)
{
    unsigned i;
    for (i=0;i<usb_driver_list_count;i++) {
        if (usb_driver_list[i]->probe(dev,intf)==0) {
            if (usbh__claim_interface(dev,intf)==0) {
                intf->drv = usb_driver_list[i];
            }
        }
    }
    return -1;
}

int usb_driver__disconnect(struct usb_device *dev, struct usb_interface *intf)
{
    if (intf->drv) {
        if (intf->drv->disconnect) {
            ESP_LOGI("USBDRIVER", "Disconnecting driver");
            intf->drv->disconnect(dev, intf);
        } else {
            ESP_LOGE("USBDRIVER", "Driver does not implement disconnect!");
        }
    } else {
        ESP_LOGE("USBDRIVER", "No driver mapped to interface");
        return -1;
    }
    return 0;
}