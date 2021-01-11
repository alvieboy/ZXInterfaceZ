#ifndef __USB_LL_H__
#define __USB_LL_H__

#include <inttypes.h>

struct usb_device_info;

typedef enum {
    PID_IN = 0,
    PID_OUT = 1,
    PID_SETUP = 3
} usb_dpid_t;

typedef enum {
    EP_TYPE_CONTROL,
    EP_TYPE_ISOCHRONOUS,
    EP_TYPE_BULK,
    EP_TYPE_INTERRUPT
} eptype_t;


void usb_ll__interrupt(void);
int usb_ll__init(void);
void usb_ll__set_power(int on);

int usb_ll__read_status(uint8_t regs[4]);
/*void usb_ll__device_addressed_callback(struct usb_device_info*);
void usb_ll__in_completed_callback(uint8_t channel, uint8_t status);
void usb_ll__out_completed_callback(uint8_t channel, uint8_t status);
void usb_ll__setup_completed_callback(uint8_t channel, uint8_t status);
*/
void usb_ll__connected_callback(uint8_t fullspeed);
void usb_ll__disconnected_callback(void);
void usb_ll__overcurrent_callback(void);

void usb_ll__channel_set_interval(uint8_t chan, uint8_t interval);

int usb_ll__submit_request(uint8_t channel, 
                           usb_dpid_t pid,
                           uint8_t *data, uint8_t datalen,
                           int (*reap)(uint8_t channel, uint8_t status,void*), void*);

int usb_ll__alloc_channel(uint8_t devaddr,
                          eptype_t eptype,
                          uint8_t maxsize,
                          uint8_t epnum,
                          void *userdata);

uint8_t usb_ll__get_channel_maxsize(uint8_t channel);
void usb_ll__set_channel_maxsize(uint8_t channel, uint8_t maxsize);
int usb_ll__reset_channel(uint8_t channel);
void usb_ll__ack_received(uint8_t channel);
void usb_ll__set_seq(uint8_t channel, uint8_t seq);
void usb_ll__set_devaddr(uint8_t channel, uint8_t addr);


int usb_ll__release_channel(uint8_t channel);
int usb_ll__read_in_block(uint8_t channel, uint8_t *target, uint8_t *rxlen);
void usb_ll__reset(void);
void usb_ll__dump_info(void);
#ifdef __linux__
void usb_ll__idle(void);
uint8_t usb_ll__get_address(void);
#endif
#endif
