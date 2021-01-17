#include "usbhub.h"
#include "usbh.h"
#include "usb_driver.h"
#include <stdlib.h>
#include "usb_descriptor.h"
#include "log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define USBHUBTAG "USBHUB"
#define USBHUBDEBUG(x...) LOG_DEBUG(DEBUG_ZONE_USBH, USBHUBTAG ,x)

static void usbhub__device_free(struct usb_device *dev)
{
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

static char *usbhub__get_device_string(struct usb_device *dev,uint16_t index)
{
    uint8_t local_string_descriptor[256];
    uint16_t size_transferred;

    if (index<1)
        return NULL;

    if (usbh__get_descriptor(dev,
                             USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                             USB_DESC_STRING | index,
                             (uint8_t*)local_string_descriptor,
                             sizeof(local_string_descriptor),
                             &size_transferred) <0) {
        return NULL;
    }
    uint8_t len = local_string_descriptor[0];
    uint8_t type = local_string_descriptor[1];
    if (type!=USB_DESC_TYPE_STRING || (len<4)) {
        ESP_LOGE(USBHUBTAG,"String descriptor NOT a string!");
        return NULL;
    }
    return usbh__string_unicode8_to_char(&local_string_descriptor[2], len - 2);
}

static int usbhub__get_device_strings(struct usb_device *dev)
{
    dev->vendor = usbhub__get_device_string(dev, dev->device_descriptor.iManufacturer);
    dev->product = usbhub__get_device_string(dev, dev->device_descriptor.iProduct);
    dev->serial = usbhub__get_device_string(dev, dev->device_descriptor.iSerialNumber);
    return 0;
}


static int usbhub__parse_interfaces(struct usb_device *dev)
{
    int index = 0;
    int intflen;

    if (dev->config_descriptor->bNumInterfaces>2) {
        // TOOO many for us.
        return -1;
    }

    usb_interface_descriptor_t *intf = usb__get_interface_descriptor(dev->config_descriptor, index++);
    usb_interface_descriptor_t *next_intf = NULL;

    if (intf==NULL) {
        ESP_LOGE(USBHUBTAG,"Cannot locate interface descriptor");
        return -1;
    }

    do {
        next_intf = usb__get_interface_descriptor(dev->config_descriptor, index++);
        if (next_intf) {
            intflen = (uint8_t*)next_intf - (uint8_t*)intf;
        } else {

            // Ugly...
            intflen =  __le16(dev->config_descriptor->wTotalLength) - ((uint8_t*)intf - (uint8_t*)dev->config_descriptor);
        }

        if ((dev->interfaces[ intf->bInterfaceNumber ].numsettings>1) || (intf->bAlternateSetting>1)) {
            // TOOO many!
            ESP_LOGE(USBHUBTAG,"Too many alternate settings");
            return -1;
        }

        dev->interfaces[ intf->bInterfaceNumber ].descriptors[ intf->bAlternateSetting ] = intf;
        dev->interfaces[ intf->bInterfaceNumber ].descriptorlen[ intf->bAlternateSetting ] = intflen;
        dev->interfaces[ intf->bInterfaceNumber ].numsettings++;
        dev->interfaces[ intf->bInterfaceNumber ].drv = NULL;
        dev->interfaces[ intf->bInterfaceNumber ].drvdata = NULL;

        intf = next_intf;
    } while (next_intf);

    return -1;
}

static uint8_t usbhub__allocate_address(struct usb_hub *hub)
{
    hub->usb_address++;
    if (hub->usb_address>127) {
        hub->usb_address = 1;
    }
    return hub->usb_address;
}

static int usbhub__assign_address(struct usb_device *dev)
{
#ifdef __linux__
    uint8_t newaddress = usb_ll__get_address();
#else
    struct usb_request req = {0};

    uint8_t newaddress = usbhub__allocate_address(dev->hub);

    req.device = dev;
    req.target = NULL;
    req.length = 0;
    req.rptr = NULL;
    req.size_transferred = 0;
    req.direction = REQ_HOST_TO_DEVICE;
    req.control = 1;

    req.setup.bmRequestType = USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD | USB_HOST_TO_DEVICE;
    req.setup.bRequest = USB_REQ_SET_ADDRESS;
    req.setup.wValue = __le16(newaddress);
    req.setup.wIndex = __le16(0);
    req.setup.wLength = __le16(0);

    req.channel =  dev->ep0_chan;
    USBHUBDEBUG("Submitting address request");
    usbh__submit_request(&req);

    // Wait.
    if (usbh__wait_completion(&req, SET_ADDRESS_DEFAULT_TIMEOUT)<0) {
        ESP_LOGE(USBHUBTAG, "Device not accepting address!");
        return -1;
    }
    USBHUBDEBUG("Submitted address request");
#endif

    usb_ll__set_devaddr(dev->ep0_chan, newaddress);
    dev->address = newaddress;

    return 0;
}

int usbhub__port_reset(struct usb_hub *hub, int port, usb_speed_t speed)
{
    struct usb_device dev;
    usb_config_descriptor_t cd;
    uint16_t size_transferred;
    bool retry_device_descriptor = false;
    int r;
    int retries;

    dev.address = 0;
    dev.claimed = 0;
    dev.vendor = NULL;
    dev.product = NULL;
    dev.serial = NULL;

    dev.ep0_chan = usb_ll__alloc_channel(dev.address,
                                         EP_TYPE_CONTROL,
                                         speed == USB_FULL_SPEED ? 64 : 8,
                                         0x00,
                                         speed==USB_FULL_SPEED?1:0,
                                         &dev
                                        );
    retries = 3;

    do {
        ESP_LOGI(USBHUBTAG, "Resetting BUS now, port %d", port);

        hub->reset(hub, port);

        ESP_LOGI(USBHUBTAG, "Resetting BUS done");


        USBHUBDEBUG("Requesting device descriptor");
        do {
            retry_device_descriptor = false;
            if (usbh__get_descriptor(&dev,
                                     USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                                     USB_DESC_DEVICE,
                                     &dev.device_descriptor_b[0],
                                     USB_LEN_DEV_DESC,
                                     &size_transferred)<0) {
                ESP_LOGE(USBHUBTAG, "Cannot fetch device descriptor");
                r = -1;
                break;
            }

            if (size_transferred<USB_LEN_DEV_DESC) {
                // Short read. EP size is probably less than 64.
                if (size_transferred<8) {
                    // Too short to evaluate EP size.
                    ESP_LOGE(USBHUBTAG,"EP0 data too small to evaluate EP size");
                    r = -1;
                    break;
                }
                // Get EP0 size
                uint8_t newep0size = dev.device_descriptor.bMaxPacketSize;
                if (newep0size==usb_ll__get_channel_maxsize(dev.ep0_chan)) {
                    ESP_LOGE(USBHUBTAG,"EP0 size inconsistent");
                    r = -1;
                    break;
                }
                usb_ll__set_channel_maxsize(dev.ep0_chan, newep0size);
                USBHUBDEBUG("Descriptor too short, retrying");
                retry_device_descriptor = true;
            } else {
                r = 0;
            }
        } while (retry_device_descriptor);

        if (r<0) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }

    } while (r<0 && retries--);

    if (r<0){
        ESP_LOGE(USBHUBTAG,"Cannot enumerate device!");
        return r;
    }

    BUFFER_LOGI(USBHUBTAG, "Device descriptor",
                &dev.device_descriptor_b[0],
                sizeof(dev.device_descriptor));


    ESP_LOGI(USBHUBTAG," new USB device, vid=0x%04x pid=0x%04x",
             __le16(dev.device_descriptor.idVendor),
             __le16(dev.device_descriptor.idProduct)
            );

    dev.hub = hub;
    dev.hub_port = port;

    // Update EP0 size

    //dev.ep0_size = dev.device_descriptor.bMaxPacketSize;

    if (usbhub__assign_address(&dev)<0) {
        return -1;
    }

    USBHUBDEBUG(" assigned address %d", dev.address);

    // Allocate channel for EP0

    dev.config_descriptor = NULL;

    USBHUBDEBUG("Allocated channel %d for EP0 of %d\n", dev.ep0_chan, dev.address);
    // Get configuration header. (9 bytes)

    // Get strings.
    usbhub__get_device_strings(&dev);

    if (usbh__get_descriptor(&dev,
                             USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                             USB_DESC_CONFIGURATION,
                             (uint8_t*)&cd,
                             USB_LEN_CFG_DESC,
                             &size_transferred) <0 || (size_transferred!=USB_LEN_CFG_DESC)) {
        ESP_LOGE(USBHUBTAG, "Cannot fetch configuration descriptor");
        return -1;
    }

    if (cd.bDescriptorType!=USB_DESC_TYPE_CONFIGURATION) {
        ESP_LOGE(USBHUBTAG,"Invalid config descriptor");
        return -1;
    }

    uint16_t configlen = __le16(cd.wTotalLength);

    dev.config_descriptor = malloc(configlen);

    if (dev.config_descriptor==NULL) {
        ESP_LOGE(USBHUBTAG,"Cannot allocate memory");
        goto err1;
    }

    if (usbh__get_descriptor(&dev,
                             USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                             USB_DESC_CONFIGURATION,
                             (uint8_t*)dev.config_descriptor,
                             configlen,
                             &size_transferred
                            )<0 || (size_transferred!=configlen)) {
        ESP_LOGE(USBHUBTAG, "Cannot read full config descriptor!");
        goto err2;
    }

    // Allocate a proper device/interfaces.
    struct usb_device *newdev = (struct usb_device*) malloc (
                                                             sizeof(struct usb_device) +
                                                             sizeof(struct usb_interface) * (dev.config_descriptor->bNumInterfaces)
                                                            );
    memcpy(newdev, &dev, sizeof(dev));

    int i;
    for (i=0;i< (dev.config_descriptor->bNumInterfaces);i++) {
        newdev->interfaces[i].numsettings = -1;
        newdev->interfaces[i].claimed = 0;
    }

    // Create the interface structures
    usbhub__parse_interfaces(newdev);

    USBHUBDEBUG("Finding USB driver");

    for (i=0;i< (dev.config_descriptor->bNumInterfaces);i++) {

        usb_driver__probe(newdev, &newdev->interfaces[i]);

    }

    if (newdev->claimed==0) {
        // No driver.
        usbhub__device_free(newdev);
    } else {
        // Add to list.
        usbdevice__add_device(newdev);
    }

    return 0;

err2:
    free(dev.config_descriptor);
err1:
    usb_ll__release_channel(dev.ep0_chan);
    return -1;
}

void usbhub__port_disconnect(struct usb_hub *hub, int port)
{
    ESP_LOGI(USBHUBTAG,"HUB port disconnect, port %d", port);
    struct usb_device *dev = usbdevice__find_by_hub_port(hub, port);
    if (dev) {
        usbdevice__disconnect(dev);
        // Release EP0
        usb_ll__release_channel(dev->ep0_chan);
    }
}

int usbhub__poweron(struct usb_hub *h)
{
    int i;
    int ports = h->get_ports(h);

    for (i=1; i<=ports;i++) {
        if (h->set_power(h, i, 1)<0)
            return -1;
    }
    return 0;
}

int usbhub__overcurrent(struct usb_hub *h, int port)
{
    ESP_LOGE(USBHUBTAG,"OVERCURRENT, disabling port");
    h->set_power(h, port, 0);
    usbhub__port_disconnect(h, port);
    return 0;
}

void usbhub__new(struct usb_hub *h)
{
    ESP_LOGI(USBHUBTAG,"New HUB detected with %d ports", h->get_ports(h));

    if (usbhub__poweron(h)<0) {

        ESP_LOGE(USBHUBTAG,"Cannot power on");
        return;
    }

    if (h->init)
        h->init(h);
}
