#include "usbdriver.h"
#include "usb_defs.h"
#include "usbh.h"

#if 0
// Still tbd

/*

    bLength                 9
    bDescriptorType         2
    wTotalLength           34
    bNumInterfaces          1
    bConfigurationValue     1
    iConfiguration          0 
    bmAttributes         0xa0
      (Bus Powered)
      Remote Wakeup
    MaxPower              100mA

    */


usb_interface_descriptor_t *usb__get_first_interface_descriptor(usb_config_descriptor_t *conf)
{
    return &conf->interfaces[0];

}

int usbhid__probe(struct usb_device *dev)
{
    usb_interface_descriptor_t *intf = usb__get_first_interface_descriptor(dev->config_descriptor);

    if (intf->bInterfaceClass == 0x03) {
    }
        //bInterfaceSubClass      1 Boot Interface Subclass
        //  bInterfaceProtocol      2 Mouse


}


int usbdriver__probe(struct usb_device *dev)
{
    return usbhid__probe(dev);
}
#else

int usbdriver__probe(struct usb_device *dev)
{
    return -1;
}

#endif
