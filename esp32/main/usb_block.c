#include "usb_driver.h"
#include "usb_defs.h"
#include "usbh.h"
#include <stdlib.h>
#include "usb_descriptor.h"
#include "usb_block.h"
#include "esp_log.h"
#include "defs.h"

#define USBBLOCKTAG "USBBLOCK"
#define USBBLOCKDEBUG(x...) ESP_LOGI(USBBLOCKTAG,x)

typedef enum {
    USB_BLOCK_STATE_IDLE
} usb_block_state_t;


struct usb_block
{
    struct usb_device *dev;
    struct usb_interface *intf;
    uint8_t max_lun;
    uint8_t in_epchan:7;
    uint8_t in_seq:1;
    uint8_t out_epchan:7;
    uint8_t out_seq:1;
    uint8_t cbw_queued;
} __attribute__((packed));

// Command Block Wrapper
struct usb_block_cbw {
    le_uint32_t dCBWSignature;
    le_uint32_t dCBWTag;
    le_uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN:4;
    uint8_t rsvd:4;
    uint8_t bCBWCBLength:5;
    uint8_t rsvd:3;
    uint8_t CBWCB[15];
} __attribute__((packed));

// Command status wrapper
struct usb_block_csw {
    le_uint32_t dCSWSignature;
    le_uint32_t dCSWTag;
    le_uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} __attribute__((packed));


#define CBWSIGNATURE (0x43425355)
#define CBW_FLAG_DEVICE_TO_HOST (0x80)
#define CBW_FLAG_HOST_TO_DEVICE (0x00)

#define USB_BLOCK_REQ_RESET (0xFF)
#define USB_BLOCK_REQ_GET_MAX_LUN (0xFE)

int usb_block__reset(struct usb_block *self)
{
    struct usb_request req = {0};
    usb_interface_descriptor_t *intf = self->intf->descriptors[0];


    req.device = self->dev;
    req.target = NULL;
    req.length = 0;
    req.rptr = NULL;
    req.size_transferred = 0;
    req.direction = REQ_HOST_TO_DEVICE;
    req.control = 1;

    req.setup.bmRequestType = USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_CLASS | USB_HOST_TO_DEVICE;
    req.setup.bRequest = USB_BLOCK_REQ_RESET;
    req.setup.wValue = 0;
    req.setup.wIndex = __le16(intf->bInterfaceNumber);
    req.setup.wLength = __le16(0);

    req.channel =  self->dev->ep0_chan;
    USBBLOCKDEBUG("Submitting reset request");
    usbh__submit_request(&req);

    if (usbh__wait_completion(&req)<0) {
        ESP_LOGE(USBBLOCKTAG,"Device not resetting!");
        return -1;
    }

    self->cbw_queued = 0;
    return 0;
}

int usb_block__get_max_lun(struct usb_block *self)
{
    struct usb_request req = {0};
    usb_interface_descriptor_t *intf = self->intf->descriptors[0];


    req.device = self->dev;
    req.target = &self->max_lun;
    req.length = 1;
    req.rptr = &self->max_lun;
    req.size_transferred = 0;
    req.direction = REQ_DEVICE_TO_HOST;
    req.control = 1;
    req.retries = 3;

    req.setup.bmRequestType = USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_CLASS | USB_DEVICE_TO_HOST;
    req.setup.bRequest = USB_BLOCK_REQ_GET_MAX_LUN;
    req.setup.wValue = 0;
    req.setup.wIndex = __le16(intf->bInterfaceNumber);
    req.setup.wLength = __le16(1);

    req.channel =  self->dev->ep0_chan;
    req.epsize =  self->dev->ep0_size;

    USBBLOCKDEBUG("Submitting GET MAX LUN request");
    usbh__submit_request(&req);

    if (usbh__wait_completion(&req)<0) {
        ESP_LOGE(USBBLOCKTAG,"Device not responding, assumming 1");
        self->max_lun = 1;
    }
    return 0;
}


static int usb_block__probe(struct usb_device *dev, struct usb_interface *i)
{
    usb_interface_descriptor_t *intf = i->descriptors[0];
    int intf_len = i->descriptorlen[0];
    bool valid = false;
    usb_endpoint_descriptor_t *ep_in = NULL;
    usb_endpoint_descriptor_t *ep_out = NULL;


    USBBLOCKDEBUG("     ***** PROBING INTERFACE %d altSetting %d **** class=%02x subclass=%02x proto=%02x\n",
                  intf->bInterfaceNumber,
                  intf->bAlternateSetting,
                  intf->bInterfaceClass,
                  intf->bInterfaceSubClass,
                  intf->bInterfaceProtocol
                 );

    if (intf->bInterfaceClass == 0x08) {
        if ((intf->bInterfaceSubClass == 0x00)|
            (intf->bInterfaceSubClass == 0x06))
        {
            if (intf->bInterfaceProtocol == 0x50) { 
                valid = true;
            }
        }
    }

    if (!valid) {
        USBBLOCKDEBUG("Not USB block device");
        return false;
    }

    int index = 0;
    usb_endpoint_descriptor_t *ep = NULL;
    do {
        ep = (usb_endpoint_descriptor_t*)usb__find_descriptor( intf, intf_len, USB_DESC_TYPE_ENDPOINT, index);
        if (ep) {
            if (ep->bmAttributes == 0x02) { // Bulk
                if (ep->bEndpointAddress & 0x80) {
                    // EP IN
                    ep_in = ep;
                } else {
                    // EP out
                    ep_out = ep;
                }
            }
        } 
        index++;
    } while (ep);


    if (!ep_in) {
        USBBLOCKDEBUG("Cannot locate IN endpoint");
        return false;
    }

    if (!ep_out) {
        USBBLOCKDEBUG("Cannot locate OUT endpoint");
        return false;
    }


    if (usbh__set_configuration(dev, dev->config_descriptor->bConfigurationValue)<0)
        return -1;

    struct usb_block *self =  malloc(sizeof(struct usb_block));

    self->dev = dev;
    self->intf = i;
    self->in_seq = 0;
    self->out_seq = 0;
    //self->ep_in = ep_in;
    //self->ep_out = ep_out;

    ESP_LOGI(USBBLOCKTAG,"Endpoint 0x%08x max size %d", ep_in->bEndpointAddress, ep_in->wMaxPacketSize);

    self->in_epchan = usb_ll__alloc_channel(dev->address,
                                            EP_TYPE_BULK,
                                            ep_in->wMaxPacketSize,
                                            ep_in->bEndpointAddress
                                           );

    self->out_epchan = usb_ll__alloc_channel(dev->address,
                                             EP_TYPE_BULK,
                                             ep_out->wMaxPacketSize,
                                             ep_out->bEndpointAddress
                                            );

    usb_ll__channel_set_interval(self->in_epchan, ep_in->bInterval);


    i->drvdata = self;

    return 0;
}

static void usb_block__disconnect(struct usb_device *dev, struct usb_interface *intf)
{
    struct usb_block *b = intf->drvdata;
    // Cancel any pending requests
    usb_ll__release_channel(b->in_epchan);
    usb_ll__release_channel(b->out_epchan);

    free(b);
    intf->drvdata = NULL;

}



const struct usb_driver usb_block_driver = {
    .probe = &usb_block__probe,
    .disconnect = &usb_block__disconnect
};



