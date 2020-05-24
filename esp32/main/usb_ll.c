#include "fpga.h"
#include "defs.h"
#include "spi.h"
#include "usb_ll.h"
#include "dump.h"
#include "usb_defs.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "usbhost_regs.h"
#include "byteorder.h"
#include <string.h>

struct chconf
{
    int (*completion)(uint8_t channel, uint8_t status, void *userdata);
    void *userdata;
    uint16_t memaddr;
};

#define MAX_USB_CHANNELS 8

static struct chconf channel_conf[MAX_USB_CHANNELS] = {0};
static uint8_t channel_alloc_bitmap = 0; // Channel 0 reserved


int usb_ll__alloc_channel(uint8_t devaddr,
                          eptype_t eptype,
                          uint8_t maxsize,
                          uint8_t epnum)
{
    uint8_t mask = 1;
    uint8_t chnum = 0;
    while ((channel_alloc_bitmap & mask)) {
        mask<<=1;
        chnum++;
    }
    if (mask==0)
        return -1; // No free channels

    channel_alloc_bitmap |= mask; // Reserve it.

    uint8_t buf[5];

    /* Configure channel */
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    fpga__write_usb_block(USB_REG_CHAN_TRANS1(chnum),buf,3);

    buf[0] = (((uint8_t)eptype)<<6) | (maxsize & 0x3F);
    buf[1] = epnum; // fixme.
    buf[2] = 0x80 | (devaddr & 0x7F);
    buf[3] = 0xFF;
    buf[4] = 0xFF;
    ESP_LOGI(TAG, "Alloc channel (%d): ", chnum);
    dump__buffer(buf, 5);
    fpga__write_usb_block(USB_REG_CHAN_CONF1(chnum), buf, 5);

    channel_conf[chnum].memaddr = 0x40 * chnum;

    return chnum;
}

int usb_ll__release_channel(uint8_t channel)
{
    uint8_t epconf[1];
    epconf[0] = 0;
    // Stop EP
    fpga__write_usb_block(USB_REG_CHAN_CONF3(channel), epconf, 1);
    channel_alloc_bitmap &= ~(1<<channel);
    return 0;
}
int usb_ll__read_status(uint8_t regs[4])
{
    int r = fpga__read_usb_block(USB_REG_STATUS, regs, 4);
    return r;
}

static void usb_ll__channel_interrupt(uint8_t channel)
{
    int intpend = fpga__read_usb(USB_REG_CHAN_INTPEND(channel));

    if (intpend<0)
        return;
#ifdef USB_LL_DEBUG
    {
        char t[64];
        t[0] = '\0';
        if (intpend&0x80) {
            strcat(t," TOG");
        }
        if (intpend&0x40) {
            strcat(t," FRM");
        }
        if (intpend&0x20) {
            strcat(t," BAB");
        }
        if (intpend&0x10) {
            strcat(t," TER");
        }
        if (intpend&0x08) {
            strcat(t," ACK");
        }
        if (intpend&0x04) {
            strcat(t," NCK");
        }
        if (intpend&0x02) {
            strcat(t," STL");
        }
        if (intpend&0x01) {
            strcat(t," CPL");
        }

        ESP_LOGI(TAG, "Channel %d intpend %02x [%s]", channel, intpend, t);
    }
#endif
    uint8_t clr = intpend;

    struct chconf *c = &channel_conf[channel];

    fpga__write_usb(USB_REG_CHAN_INTCLR(channel), clr);

    c->completion( channel, clr, c->userdata );
}

void usb_ll__interrupt()
{
    uint8_t regs[4];
#ifdef USB_LL_DEBUG
    ESP_LOGI(TAG, "USB interrupt");
#endif
    if (usb_ll__read_status(regs)==0) {
#ifdef USB_LL_DEBUG
        ESP_LOGI(TAG," Status regs %02x %02x %02x %02x",
                 regs[0],
                 regs[1],
                 regs[2],
                 regs[3]);
#endif
    }

    if (regs[1] & USB_INTPEND_CONN) {
        usb_ll__connected_callback();
    }

    if (regs[1] & USB_INTPEND_DISC) {
#ifdef USB_LL_DEBUG
        ESP_LOGI(TAG,"USB disconnection event");
#endif
        usb_ll__disconnected_callback();
    }

    if (regs[1] & USB_INTPEND_OVERCURRENT) {
#ifdef USB_LL_DEBUG
        ESP_LOGI(TAG,"USB overcurrent event");
#endif
        usb_ll__overcurrent_callback();
    }

    uint8_t chanint = regs[2];
    uint8_t chanidx=0;
    while (chanint) {
        if (chanint&1) {
            usb_ll__channel_interrupt(chanidx);
        }
        chanidx++;
        chanint>>=1;
    }

    fpga__write_usb(USB_REG_INTCLR, regs[1] | USB_INTACK);
}


int usb_ll__submit_request(uint8_t channel, uint16_t epmemaddr,
                           usb_dpid_t pid, uint8_t seq, uint8_t *data, uint8_t datalen,
                          int (*reap)(uint8_t channel, uint8_t status, void*), void*userdata)
{
    uint8_t regconf[3];

    // Write epmem data
    if (data!=NULL) {
        fpga__write_usb_block(USB_REG_DATA(epmemaddr), data, datalen);
    }
    uint8_t retries = 3;

    regconf[0] = (uint8_t)pid | ((seq&1)<<2) | ((epmemaddr>>8)<<3) | (retries << 5);
    regconf[1] = epmemaddr & 0xFF;
    regconf[2] = 0x80 | (datalen & 0x7F);

    dump__buffer(regconf, 3);

    channel_conf[channel].completion = reap;
    channel_conf[channel].userdata = userdata;
    channel_conf[channel].memaddr = epmemaddr;

    fpga__write_usb_block(USB_REG_CHAN_TRANS1(channel), regconf, 3);

    return 0;
}

int usb_ll__read_in_block(uint8_t channel, uint8_t *target, uint8_t *rxlen)
{
    uint8_t trans_size;
    uint8_t inlen = 0;

    fpga__read_usb_block(USB_REG_CHAN_TRANS3(channel), &trans_size, 1);
    trans_size &= 0x7F;
    // Read memory address
    if (trans_size>0) {
        if (trans_size>64) {
            ESP_LOGW(TAG,"EP sent more than 64 bytes!!!");
            trans_size=64;
        }
        inlen = trans_size;
        ESP_LOGI(TAG, "Reading IN block from %04x", channel_conf[channel].memaddr);
        fpga__read_usb_block(USB_REG_DATA( channel_conf[channel].memaddr ), target, inlen);
        ESP_LOGI(TAG,"Response from device:");
        dump__buffer(target,inlen);
    }
    *rxlen = inlen;
    return 0;
}

void usb_ll__set_power(int on)
{
    if (on) {
        fpga__write_usb(USB_REG_STATUS, USB_STATUS_PWRON);
    } else {
        fpga__write_usb(USB_REG_STATUS, 0);
    }
}

int usb_ll__init()
{
    ESP_LOGI(TAG, "USB: Low level init");
    // Power off, enable interrupts
    uint8_t regval[4] = {
        0x00,//(USB_STATUS_PWRON),
        0x00, // Unused,
        (USB_INTCONF_GINTEN | USB_INTCONF_DISC | USB_INTCONF_CONN | USB_INTCONF_OVERCURRENT),
        0xff // Clear pending
    };

    fpga__write_usb_block(USB_REG_STATUS, regval, 4);

    usb_ll__alloc_channel(0x00,
                          EP_TYPE_CONTROL,
                          64,
                          0);
    // Set up chan0
    //channel_conf[0].memaddr = 0x0000;

    return 0;
}

void usb_ll__reset()
{
    // Send reset.
    fpga__write_usb(USB_REG_STATUS, (USB_STATUS_PWRON |USB_STATUS_RESET));
}