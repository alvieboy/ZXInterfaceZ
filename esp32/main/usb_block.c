#include "usb_driver.h"
#include "usb_defs.h"
#include "usbh.h"
#include <stdlib.h>
#include "usb_descriptor.h"
#include "usb_block.h"
#include "esp_log.h"
#include "defs.h"
#include <string.h>
#include "scsidev.h"
#include "struct_assert.h"
#include "log.h"

#define USBBLOCKTAG "USBBLOCK"
#define USBBLOCKDEBUG(x...) ESP_LOGI(USBBLOCKTAG,x)

typedef enum {
    USB_BLOCK_STATE_GET_MAX_LUN,
    USB_BLOCK_STATE_RUNNING,
} usb_block_state_t;

// Command Block Wrapper
typedef struct {
    union {
        struct {
            le_uint32_t dCBWSignature;
            le_uint32_t dCBWTag;
            le_uint32_t dCBWDataTransferLength;
            uint8_t bmCBWFlags;
            uint8_t bCBWLUN:4;
            uint8_t rsvd:4;
            uint8_t bCBWCBLength:5;
            uint8_t rsvd2:3;
            uint8_t CBWCB[16];
        } __attribute__((packed));
        uint8_t b[31];
    };
} __attribute__((packed)) usb_block_cbw_t;

ASSERT_STRUCT_SIZE(usb_block_cbw_t, 31);

// Command status wrapper
typedef struct {
    union {
        struct {
            le_uint32_t dCSWSignature;
            le_uint32_t dCSWTag;
            le_uint32_t dCSWDataResidue;
            uint8_t bCSWStatus;
        } __attribute__((packed));
        uint8_t b[13];
    };
} __attribute__((packed)) usb_block_csw_t;

ASSERT_STRUCT_SIZE(usb_block_csw_t, 13);


struct usb_block
{
    struct usb_device *dev;
    struct usb_interface *intf;
    usb_block_state_t state;
    uint8_t max_lun;
    uint8_t in_epchan:7;
    uint8_t in_seq:1;
    uint8_t out_epchan:7;
    uint8_t out_seq:1;
    uint8_t cbw_queued;
    usb_block_cbw_t cbw;

    scsidev_t scsidev;
    uint32_t tag;
} __attribute__((packed));




#define CBWSIGNATURE (0x43425355)
#define CBW_FLAG_DEVICE_TO_HOST (0x80)
#define CBW_FLAG_HOST_TO_DEVICE (0x00)

#define USB_BLOCK_REQ_RESET (0xFF)
#define USB_BLOCK_REQ_GET_MAX_LUN (0xFE)

#define CSWSIGNATURE (0x53425355)


static int usb_block__check_csw(const usb_block_csw_t *csw, uint32_t tag, uint8_t *status_out);

#define USBBLOCK_DEFAULT_REQUEST_TIMEOUT (1000/portTICK_RATE_MS)

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
    req.retries = 3;

    req.setup.bmRequestType = USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_CLASS | USB_HOST_TO_DEVICE;
    req.setup.bRequest = USB_BLOCK_REQ_RESET;
    req.setup.wValue = 0;
    req.setup.wIndex = __le16(intf->bInterfaceNumber);
    req.setup.wLength = __le16(0);

    req.channel =  self->dev->ep0_chan;
    USBBLOCKDEBUG("Submitting reset request");
    usbh__submit_request(&req);

    if (usbh__wait_completion(&req, USBBLOCK_DEFAULT_REQUEST_TIMEOUT )<0) {
        ESP_LOGE(USBBLOCKTAG,"Device not resetting!");
        return -1;
    }

    self->cbw_queued = 0;
    return 0;
}

int usb_block__get_max_lun(struct usb_block *self)
{
    struct usb_request req = {0};
    uint8_t buf[8];

    usb_interface_descriptor_t *intf = self->intf->descriptors[0];


    req.device = self->dev;
    req.target = buf;
    req.length = 1;
    req.rptr = buf;
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
    //req.epsize =  self->dev->ep0_size;

    USBBLOCKDEBUG("Submitting GET MAX LUN request");
    usbh__submit_request(&req);

    if (usbh__wait_completion(&req, USBBLOCK_DEFAULT_REQUEST_TIMEOUT)<0) {
        ESP_LOGE(USBBLOCKTAG,"Device not responding");
        self->max_lun = -1;
        return -1;
    }

    self->max_lun = buf[0];

    USBBLOCKDEBUG("Max LUN: %d\n",self->max_lun);
    return 0;
}

/*static int usb_block__inquiry()
{
   return -1;
}
static int usb_block__check_unit_ready()
{
    return -1;
}
static int usb_block__read_capacity()
{
    return -1;
}
*/
static int usb_block__scsi_write(void *pvt, uint8_t *cdb, unsigned size, const uint8_t *data, unsigned targetlen, uint8_t *status_out);
static int usb_block__scsi_read(void *pvt, uint8_t *cdb, unsigned size, uint8_t *data, unsigned targetlen, uint8_t *status_out);

const struct scsiblockfn usb_block_scsidev_fn = {
    .write = &usb_block__scsi_write,
    .read = &usb_block__scsi_read,
};


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
    self->tag = 0;
    //self->ep_in = ep_in;
    //self->ep_out = ep_out;

    ESP_LOGI(USBBLOCKTAG,"Endpoint 0x%08x max size %d", ep_in->bEndpointAddress, ep_in->wMaxPacketSize);

    self->in_epchan = usb_ll__alloc_channel(dev->address,
                                            EP_TYPE_BULK,
                                            ep_in->wMaxPacketSize,
                                            ep_in->bEndpointAddress,
                                            dev->fullspeed,
                                            dev
                                           );

    self->out_epchan = usb_ll__alloc_channel(dev->address,
                                             EP_TYPE_BULK,
                                             ep_out->wMaxPacketSize,
                                             ep_out->bEndpointAddress,
                                             dev->fullspeed,
                                             dev
                                            );

    ESP_LOGI(USBBLOCKTAG,"In chan %d, out chan %d", self->in_epchan, self->out_epchan);

    usb_ll__channel_set_interval(self->in_epchan, ep_in->bInterval);

    if (usb_block__get_max_lun(self)<0) {
        ESP_LOGE(USBBLOCKTAG,"Cannot get max LUN from device");

        usb_ll__release_channel(self->in_epchan);
        usb_ll__release_channel(self->out_epchan);

        free(self);
        return -1;
    }

    ESP_LOGI(USBBLOCKTAG,"Initialising SCSI layer, usb block %p", self);


    strcpy(self->scsidev.backend, "usb");

    scsidev__init(&self->scsidev, &usb_block_scsidev_fn, self);

    // Register block device

    i->drvdata = self;

    return 0;
}

static void usb_block__disconnect(struct usb_device *dev, struct usb_interface *intf)
{
    struct usb_block *b = intf->drvdata;
    // Cancel any pending requests
    usb_ll__release_channel(b->in_epchan);
    usb_ll__release_channel(b->out_epchan);

    scsidev__deinit(&b->scsidev);

    free(b);
    intf->drvdata = NULL;

}

int usb_block__command_transfer_completed(uint8_t channel, uint8_t status,void*self)
{
    struct usb_block *blk = (struct usb_block *)self;
    if (channel == blk->out_epchan) {

    } else if (channel == blk->in_epchan) {
        
    } else {
        // error
    }
    return 0;
}

static int usb_block__send_command(struct usb_block *blk,
                                   uint8_t lun,
                                   const uint8_t *command_data,
                                   uint8_t command_len,
                                   unsigned datalen,
                                   uint8_t flags)
{
    if (command_len <1  || command_len>16) {
        ESP_LOGE(USBBLOCKTAG,"Invalid command size %d", command_len);
        return -1;
    }
    blk->cbw.dCBWSignature = __le32(CBWSIGNATURE);
    blk->cbw.dCBWTag = __le32(blk->tag);
    blk->cbw.dCBWDataTransferLength = __le32(datalen);

    blk->cbw.bmCBWFlags = flags;
    blk->cbw.bCBWLUN = lun;
    blk->cbw.rsvd = 0;
    blk->cbw.bCBWCBLength = command_len;
    blk->cbw.rsvd2=0;

    memcpy(&blk->cbw.CBWCB[0], command_data, command_len);
    if (command_len<16) {
        memset(&blk->cbw.CBWCB[command_len], 0x00, 16-command_len);
    }

    //ESP_LOGI(USBBLOCKTAG,"Sending request channel %d command_len %d", blk->out_epchan, command_len);

    struct usb_request req = {0};

    req.device = blk->dev;
    //req.seq = blk->out_seq;
    req.target = &blk->cbw.b[0];
    req.length = sizeof(blk->cbw);
    req.rptr = NULL;
    req.size_transferred = 0;
    req.direction = REQ_HOST_TO_DEVICE;
    req.control = 0;
    req.channel = blk->out_epchan;
    req.retries = 3;

    //req.epsize  = usb_ll__get_channel_maxsize(blk->out_epchan);

    BUFFER_LOGI(USBBLOCKTAG,"CBW", blk->cbw.b, sizeof(blk->cbw));




    usbh__submit_request(&req);
    int r = usbh__wait_completion(&req, USBBLOCK_DEFAULT_REQUEST_TIMEOUT);

    if (r==0) {
        blk->tag++;
    }

    //ESP_LOGI(USBBLOCKTAG,"Request completed on channel %d", blk->out_epchan);

    return r;
}



static int usb_block__send_data(struct usb_block *blk,
                                uint8_t lun,
                                uint8_t *data,
                                unsigned datalen)
{
    //ESP_LOGI(USBBLOCKTAG,"Sending OUT request channel %d len %d", blk->out_epchan, datalen);

    struct usb_request req = {0};

    req.device = blk->dev;
    //req.seq = blk->out_seq;
    req.target = data,
    req.length = datalen,
    req.rptr = data;
    req.size_transferred = 0;
    req.direction = REQ_HOST_TO_DEVICE;
    req.control = 0;
    req.channel = blk->out_epchan;
    req.retries = 3;
    //req.epsize  = usb_ll__get_channel_maxsize(blk->out_epchan);


    usbh__submit_request(&req);
    int r = usbh__wait_completion(&req, USBBLOCK_DEFAULT_REQUEST_TIMEOUT);

    return r;
}

static int usb_block__wait_reply(struct usb_block *blk,
                                uint8_t lun,
                                uint8_t *resp,
                                unsigned datalen)
{
    //ESP_LOGI(USBBLOCKTAG,"Sending IN request channel %d len %d", blk->out_epchan, datalen);

    struct usb_request req = {0};

    req.device = blk->dev;
    req.target = resp,
    req.length = datalen,
    req.rptr = resp;
    req.size_transferred = 0;
    req.direction = REQ_DEVICE_TO_HOST;
    req.control = 0;
    req.channel = blk->in_epchan;
    //req.epsize  = usb_ll__get_channel_maxsize(blk->out_epchan);


    usbh__submit_request(&req);
    int r = usbh__wait_completion(&req, USBBLOCK_DEFAULT_REQUEST_TIMEOUT);

    return r;
}






static int usb_block__scsi_read(void *pvt,
                                uint8_t *cdb,
                                unsigned size,
                                uint8_t *rx_target,
                                unsigned rx_targetlen,
                                uint8_t *status_out)
{
    struct usb_block *self = (struct usb_block *)pvt;
    //ESP_LOGI(USBBLOCKTAG,"Request READ self=%p", self);
    usb_block_csw_t csw;

    uint32_t tag = self->tag;
    int r = usb_block__send_command(self,
                                    0x00, // LUNself->lun,
                                    cdb,
                                    size,
                                    rx_targetlen,
                                    CBW_FLAG_DEVICE_TO_HOST
                                   );


    if (r<0)
        return r;

    if (rx_targetlen) {
        // Data phase
        r = usb_block__wait_reply(self,
                                  0x00, // LUNuint8_t lun,
                                  rx_target,
                                  rx_targetlen);
    }

    //ESP_LOGI(USBBLOCKTAG, "Request CSW into %p", &csw);

    r = usb_block__wait_reply(self,
                              0x00, // LUNuint8_t lun,
                              &csw.b[0],
                              sizeof(csw));

    // Verify CSW
    if (r<0) {
        return -1;
    }

    return usb_block__check_csw(&csw, tag, status_out);

}

static int usb_block__check_csw(const usb_block_csw_t *csw, uint32_t tag, uint8_t *status_out)
{
    if (__le32(csw->dCSWSignature)!=CSWSIGNATURE) {
        ESP_LOGI(USBBLOCKTAG,"Invalid CSW signature %08x", __le32(csw->dCSWSignature));
        return -1;
    }

    if (__le32(csw->dCSWTag)!=tag) {
        ESP_LOGI(USBBLOCKTAG,"Invalid TAG");
        return -1;
    }

    *status_out = csw->bCSWStatus;
    return 0;
}

static int usb_block__scsi_write(void *pvt,
                                uint8_t *cdb,
                                unsigned size,
                                const uint8_t *tx_source,
                                unsigned tx_len,
                                uint8_t *status_out)
{
    struct usb_block *self = (struct usb_block *)pvt;

    usb_block_csw_t csw;

    uint32_t tag = self->tag;

    int r = usb_block__send_command(self,
                                    0x00, // LUNself->lun,
                                    cdb,
                                    size,
                                    tx_len,
                                    CBW_FLAG_HOST_TO_DEVICE);


    if (r<0)
        return r;

    if (tx_len) {
        // Data phase
        r = usb_block__send_data(self,
                                  0x00, // LUNuint8_t lun,
                                  (uint8_t*)tx_source,
                                  tx_len);
    }

    r = usb_block__wait_reply(self,
                              0x00, // LUNuint8_t lun,
                              (uint8_t*)&csw,
                              sizeof(csw));

    if (r<0)
        return r;

    return usb_block__check_csw(&csw, tag, status_out);
}





const struct usb_driver usb_block_driver = {
    .probe = &usb_block__probe,
    .disconnect = &usb_block__disconnect
};



