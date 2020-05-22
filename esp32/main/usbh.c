#include "usb_ll.h"
#include "dump.h"
#include "usb_defs.h"
#include "esp_log.h"
#include "defs.h"

int usbh__init()
{
    return usb_ll__init();
}

typedef enum {
    GET_CONFIG1,
    GET_CONFIG2
} host_state_t;

struct usb_device
{
    uint8_t ep0_chan;
    uint8_t address;
    usb_config_descriptor_t config_descriptor_short;
    usb_config_descriptor_t *config_descriptor;
    host_state_t state;
};

#define REQ_DEVICE_TO_HOST 0
#define REQ_HOST_TO_DEVICE 1

struct usb_request
{
    struct usb_device *device;
    uint8_t *target;
    uint8_t *rptr;
    uint16_t length;
    uint16_t size_transferred;
    usb_setup_t setup; // For setup requests
    uint8_t control:1;
    uint8_t direction:1;
    uint8_t is_setup:1;
};

static void usbh__request_completed(struct usb_request *req);


static void usbh__request_failed(struct usb_request *req)
{
    //switch (req->device->state) {
    //}
    free(req);
}

int usbh__request_complete_reply(uint8_t chan, uint8_t stat, void *data)
{
    struct usb_request *req = (struct usb_request*)data;
    uint8_t rxlen = 0;

    if (stat & 1) {

        if ((!req->is_setup) && (req->direction==REQ_DEVICE_TO_HOST)) {
            usb_ll__read_in_block(chan, req->target, &rxlen);
        }

        if (req->is_setup) {
            req->is_setup = 0;
            req->size_transferred = 0;
        } else {
            //unsigned transfer_size = req->length - req->size_transferred;
            //if (transfer_size > 64)
            //    transfer_size = 64;
            req->rptr += rxlen;
            req->size_transferred += rxlen;//transfer_size;
        }

        unsigned to_go = req->length - req->size_transferred;

        if (to_go>0) {
            if (to_go>64)
                to_go=64;

            if (req->direction==REQ_HOST_TO_DEVICE) {
                return usb_ll__submit_request(chan,
                                              PID_OUT,
                                              0,
                                              req->rptr, to_go,
                                              usbh__request_complete_reply,
                                              req);
            } else {
                return usb_ll__submit_request(chan,
                                              PID_IN, 1,
                                              req->rptr,
                                              to_go,
                                              usbh__request_complete_reply,
                                              req);
            }
        } else {
            // Request completed.
            usbh__request_completed(req);
        }


    } else {
        ESP_LOGE(TAG, "Request failed");
        usbh__request_failed(req);
    }

    return 0;
}

void usbh__submit_request(struct usb_request *req, uint8_t chan)
{
    if (req->control) {
        req->is_setup = 1;
        usb_ll__submit_request(chan, PID_SETUP, 0,
                               req->setup.data,
                               sizeof(req->setup),
                               usbh__request_complete_reply,
                               req);
    } else {
        if (req->direction==REQ_DEVICE_TO_HOST) {
            usb_ll__submit_request(chan, PID_IN, 0,
                                   req->target,
                                   req->length > 64 ? 64: req->length,
                                   usbh__request_complete_reply,
                                   req);
        } else {
            usb_ll__submit_request(chan, PID_OUT, 0,
                                   req->target,
                                   req->length > 64 ? 64: req->length,
                                   usbh__request_complete_reply,
                                   req);
        }

    }
}

int usbh__get_descriptor(struct usb_device *dev,
                         uint8_t  req_type,
                         uint16_t value,
                         uint8_t* target,
                         uint16_t length)
{

    struct usb_request *req = malloc(sizeof(struct usb_request));
    req->device = dev;
    req->target = target;
    req->length = length;
    req->rptr = target;
    req->size_transferred = 0;
    req->direction = REQ_DEVICE_TO_HOST;
    req->control = 1;

    req->setup.bmRequestType = req_type | USB_DEVICE_TO_HOST;
    req->setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    req->setup.wValue = __le16(value);
    if ((value & 0xff00) == USB_DESC_STRING)
    {
        // Language
        req->setup.wIndex = __le16(0x0409);
    }
    else
    {
        req->setup.wIndex = __le16(0);
    }
    req->setup.wLength = length;

    usbh__submit_request(req, dev->ep0_chan);
    return 0;
}




int usb_ll__device_addressed(struct usb_device_info*info)
{
    ESP_LOGI(TAG, "New USB device with address %d", info->address);

    if (info->desc.bDescriptorType != USB_DESC_TYPE_DEVICE) {
        ESP_LOGE(TAG, "Invalid infoice descriptor type 0x%02x", info->desc.bDescriptorType);
        return -1;
    }

    ESP_LOGI(TAG," vid=0x%04x pid=0x%04x",
             __le16(info->desc.idVendor),
             __le16(info->desc.idProduct)
            );

    // Allocate device.
    struct usb_device *dev = malloc(sizeof(struct usb_device));

    // Allocate channel for EP0

    dev->ep0_chan = usb_ll__alloc_channel(info->address,
                                          EP_TYPE_CONTROL,
                                          info->desc.bMaxPacketSize,
                                          0x00
                                          );
    dev->address = info->address;
    dev->config_descriptor = NULL;

    ESP_LOGI(TAG, "Allocated channel %d for EP0 of %d\n", dev->ep0_chan, info->address);
    // Get configuration header. (9 bytes)

    dev->state = GET_CONFIG1;

    usbh__get_descriptor(dev,
                         USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                         USB_DESC_CONFIGURATION,
                         (uint8_t*)&dev->config_descriptor_short,
                         USB_LEN_CFG_DESC);
    return 0;
}

static void usbh__request_completed(struct usb_request *req)
{
    struct usb_device *dev = req->device;

    switch (dev->state) {
    case GET_CONFIG1:
        // Analyse config descriptor
        ESP_LOGI(TAG, "Config descriptor (short):");
        dump__buffer((uint8_t*)&req->device->config_descriptor_short,
                    sizeof(req->device->config_descriptor_short));

        if (req->device->config_descriptor_short.bDescriptorType!=USB_DESC_TYPE_CONFIGURATION) {
            ESP_LOGE(TAG,"Invalid config descriptor");
        } else {
            // Allocate and read full descriptor
            if (dev->config_descriptor!=NULL)
                free(dev->config_descriptor);

            uint16_t configlen = req->device->config_descriptor_short.wTotalLength;

            dev->config_descriptor = malloc(configlen);
            if (dev->config_descriptor==NULL) {
                // TBD
                free(req);
                return;
            }

            dev->state = GET_CONFIG2;

            usbh__get_descriptor(dev,
                                 USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                                 USB_DESC_CONFIGURATION,
                                 (uint8_t*)dev->config_descriptor,
                                 configlen
                                );

        }
        break;
    case GET_CONFIG2:
        // Full config descriptor.
        ESP_LOGI(TAG,"Got full config descriptor");
        break;
    }
    free(req);
}
