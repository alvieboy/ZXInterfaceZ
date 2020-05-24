#ifndef __USBH_H__
#define __USBH_H__

#include "usb_ll.h"
#include "usb_defs.h"

typedef enum {
    DETACHED,
    GET_CONFIG1,
    GET_CONFIG2
} host_state_t;

struct usb_device
{
    uint8_t ep0_chan;
    uint8_t ep0_size;
    uint8_t address;
    usb_device_descriptor_t device_descriptor;
    usb_config_descriptor_t config_descriptor_short;
    usb_config_descriptor_t *config_descriptor;
    host_state_t state;
};

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
    uint16_t epmemaddr;
    uint8_t epsize;
    usb_setup_t setup; // For setup requests
    uint8_t control:1;
    uint8_t direction:1;
    uint8_t control_state:2;
    uint8_t channel:3;
};


int usbh__init(void);

int usbh__get_descriptor(struct usb_device *dev,
                         uint8_t  req_type,
                         uint16_t value,
                         uint8_t* target,
                         uint16_t length);
#endif
