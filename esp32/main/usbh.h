#ifndef __USBH_H__
#define __USBH_H__

#include "esp_attr.h"
#include "usb_ll.h"
#include "usb_defs.h"
#include "usb_device.h"

typedef enum {
    DETACHED,
    GET_CONFIG1,
    GET_CONFIG2
} host_state_t;

#define REQ_DEVICE_TO_HOST 0
#define REQ_HOST_TO_DEVICE 1

#define CONTROL_STATE_SETUP 0
#define CONTROL_STATE_DATA  1
#define CONTROL_STATE_STATUS 2

struct usb_request
{
    struct usb_device *device;
    uint8_t *target;
    uint8_t *rptr;
    uint16_t length;
    uint16_t size_transferred;
    //uint16_t epmemaddr;
    //uint8_t epsize;
    uint8_t retries;
    usb_setup_t setup; // For setup requests
    //uint8_t seq:1;
    uint8_t control:1;
    uint8_t direction:1;
    uint8_t control_state:2;
    uint8_t channel:4;
#ifdef __linux__
    void *pvt;
#endif
};

int usbh__init(void);

int usbh__get_descriptor(struct usb_device *dev,
                         uint8_t  req_type,
                         uint16_t value,
                         uint8_t* target,
                         uint16_t length,
                         uint16_t *size_transferred);

int usbh__claim_interface(struct usb_device *dev, struct usb_interface *intf);
int usbh__set_configuration(struct usb_device *dev, uint8_t configidx);
void usbh__dump_info(void);
void usbh__submit_request(struct usb_request *req);
int usbh__wait_completion(struct usb_request *req);
uint32_t usbh__get_device_id(const struct usb_device*dev);

const struct usb_device_entry *usbh__get_devices(void);
uint32_t usbh__get_device_id(const struct usb_device *dev);

void IRAM_ATTR usb__isr_handler(void* arg);
char *usbh__string_unicode8_to_char(const uint8_t  *src, unsigned len);

#endif
