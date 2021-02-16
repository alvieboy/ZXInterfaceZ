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
#include "log.h"


#define USBLLTAG "USBLL"

#define USBLLDEBUG(x...) LOG_DEBUG(DEBUG_ZONE_USBLL, USBLLTAG, x)

struct chconf
{
    int (*completion)(uint8_t channel, uint8_t status, void *userdata);
    void *userdata;
    uint16_t memaddr;
    uint8_t maxsize;
    uint8_t epnum;
    uint8_t eptype;
    uint8_t seq:1;
    uint8_t fullspeed:1;
    uint8_t active:1;
};

#define MAX_USB_CHANNELS 8

static struct chconf channel_conf[MAX_USB_CHANNELS] = {0};
static uint8_t channel_alloc_bitmap = 0; // Channel 0 reserved

uint8_t usb_ll__get_channel_maxsize(uint8_t channel)
{
    return channel_conf[channel].maxsize;
}

void usb_ll__set_channel_maxsize(uint8_t channel, uint8_t maxsize)
{
    channel_conf[channel].maxsize = maxsize;
}

void usb_ll__set_devaddr(uint8_t channel, uint8_t addr)
{
    uint8_t val = 0x80 | (addr & 0x7F);

    fpga__write_usb_block(USB_REG_CHAN_CONF3(channel), &val, 1);

}

void usb_ll__channel_set_interval(uint8_t chan, uint8_t interval)
{
    fpga__write_usb(USB_REG_CHAN_CONF4(chan), interval);
}

int usb_ll__alloc_channel(uint8_t devaddr,
                          eptype_t eptype,
                          uint8_t maxsize,
                          uint8_t epnum,
                          uint8_t fullspeed,
                          void *userdata)
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
    uint8_t c1 = epnum;
    if (!fullspeed)
        c1 |= (1<<6); // Low-speed indicator

    buf[1] = c1; // fixme.
    buf[2] = 0x80 | (devaddr & 0x7F);
    buf[3] = 0xFF;
    buf[4] = 0xFF;

    USBLLDEBUG("Alloc channel (%d): ", chnum);

    if (DEBUG_ENABLED(DEBUG_ZONE_USBLL)) {
        dump__buffer(buf, 5);
    }
    fpga__write_usb_block(USB_REG_CHAN_CONF1(chnum), buf, 5);

    channel_conf[chnum].memaddr = 0x40 * chnum;
    channel_conf[chnum].maxsize = maxsize;
    channel_conf[chnum].epnum = epnum;
    channel_conf[chnum].eptype = eptype;
    channel_conf[chnum].fullspeed = fullspeed;
    channel_conf[chnum].active = 0;

    usb_ll__reset_channel(chnum);

    return chnum;
}

int usb_ll__release_channel(uint8_t channel)
{
    // Stop EP
    fpga__write_usb(USB_REG_CHAN_CONF3(channel), 0);

    struct chconf *c = &channel_conf[channel];
    channel_alloc_bitmap &= ~(1<<channel);

    if (c->active) {
        c->active = false;
        c->completion( channel, 0xff, c->userdata );
    }

    return 0;
}

int usb_ll__read_status(uint8_t regs[4])
{
    int r = fpga__read_usb_block(USB_REG_STATUS, regs, 4);
    return r;
}

int usb_ll__reset_channel(uint8_t channel)
{
    channel_conf[channel].seq = channel_conf[channel].eptype==EP_TYPE_CONTROL?1:0;
    return 0;
}

void usb_ll__set_seq(uint8_t channel, uint8_t seq)
{
    channel_conf[channel].seq = seq&1;
}

static void usb_ll__channel_interrupt(uint8_t channel)
{
    int intpend = fpga__read_usb(USB_REG_CHAN_INTPEND(channel));

    if (intpend<0)
        return;
//#ifdef ENABLE_DEBUG_USBLL
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

        USBLLDEBUG("Channel %d intpend %02x [%s]", channel, intpend, t);
    }
//#endif
    uint8_t clr = intpend;

    struct chconf *c = &channel_conf[channel];

    fpga__write_usb(USB_REG_CHAN_INTCLR(channel), clr);

    if (clr & 0x01) {
        // Completed.
        usb_ll__ack_received(channel);
    }
    c->active = 0;
    c->completion( channel, clr, c->userdata );
}

void usb_ll__interrupt()
{
    uint8_t regs[4];
    USBLLDEBUG("USB interrupt");
    if (usb_ll__read_status(regs)==0) {
        USBLLDEBUG(" Status regs %02x %02x %02x %02x",
                 regs[0],
                 regs[1],
                 regs[2],
                 regs[3]);
    }


    if (regs[1] & USB_INTPEND_CONN) {
        usb_ll__connected_callback((regs[0] >> 6) & 1);
    }

    if (regs[1] & USB_INTPEND_DISC) {
        USBLLDEBUG("USB disconnection event");
        usb_ll__disconnected_callback();
    }

    if (regs[1] & USB_INTPEND_OVERCURRENT) {
        USBLLDEBUG("USB overcurrent event");
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


void usb_ll__ack_received(uint8_t channel)
{
    struct chconf *conf = &channel_conf[channel];
//    if (conf->eptype !=EP_TYPE_CONTROL) {
    conf->seq = !conf->seq;
    //    }
}

int usb_ll__submit_request(uint8_t channel,
                           usb_dpid_t pid,
                           const uint8_t *txdata, uint8_t datalen,
                           int (*reap)(uint8_t channel, uint8_t status, void*), void*userdata)
{
    uint8_t regconf[3];
    struct chconf *conf = &channel_conf[channel];

    conf->active = 1;

    USBLLDEBUG("Submitting request EP=%d PID %d seq=%d fs=%d", conf->epnum, (int)pid, conf->seq, conf->fullspeed);
    // Write epmem data
    if ((pid != PID_IN) && (txdata!=NULL)) {
        USBLLDEBUG("Copying data %p -> epmem@%04x", txdata, conf->memaddr);
        if (DEBUG_ENABLED(DEBUG_ZONE_USBLL)) {
            dump__buffer(txdata,datalen);
        }
        fpga__write_usb_block(USB_REG_DATA(conf->memaddr), txdata, datalen);
    }
    uint8_t retries = 3;

    regconf[0] = (uint8_t)pid | ((conf->seq&1)<<2) | ((conf->memaddr>>8)<<3) | (retries << 5);
    regconf[1] = conf->memaddr & 0xFF;
    regconf[2] = 0x80 | (datalen & 0x7F);


    conf->completion = reap;
    conf->userdata = userdata;
    //channel_conf[channel].memaddr = epmemaddr;

    fpga__write_usb_block(USB_REG_CHAN_TRANS1(channel), regconf, 3);

    if (DEBUG_ENABLED(DEBUG_ZONE_USBLL)) {
        fpga__read_usb_block(USB_REG_CHAN_TRANS1(channel), regconf, 3);

        dump__buffer(regconf, 3);
    }

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
        if (trans_size>(*rxlen)) {
            ESP_LOGW(TAG,"EP sent more than expected (%d, %d)!!!", trans_size, *rxlen);
            trans_size = *rxlen;
        }

        inlen = trans_size;
        USBLLDEBUG( "Reading IN block from %04x (%d bytes)", channel_conf[channel].memaddr, trans_size);
        fpga__read_usb_block(USB_REG_DATA( channel_conf[channel].memaddr ), target, inlen);
        USBLLDEBUG("Response from device:");
        if (DEBUG_ENABLED(DEBUG_ZONE_USBLL)) {
            dump__buffer(target,inlen);
        }
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
    USBLLDEBUG( "SET power: register contents %02x", fpga__read_usb(USB_REG_STATUS));
}

int usb_ll__init()
{
    USBLLDEBUG( "USB: Low level init");
    // Power off, enable interrupts
    uint8_t regval[4] = {
        0x00,//(USB_STATUS_PWRON),
        0x00, // Unused,
        (USB_INTCONF_GINTEN | USB_INTCONF_DISC | USB_INTCONF_CONN),// | USB_INTCONF_OVERCURRENT),
        0xff // Clear pending
    };

    fpga__write_usb_block(USB_REG_STATUS, regval, 4);

    return 0;
}

void usb_ll__reset()
{
    // Send reset.
    fpga__write_usb(USB_REG_STATUS, (USB_STATUS_PWRON |USB_STATUS_RESET));
}

void usb_ll__dump_info()
{
    uint8_t regs[16];
    int i;
    fpga__read_usb_block(USB_REG_STATUS, regs, 16);
    USBLLDEBUG("FPGA register information:");
    USBLLDEBUG("  Status   : 0x%02x", regs[0]);
    USBLLDEBUG("  Intpend1 : 0x%02x", regs[1]);
    USBLLDEBUG("  Intpend2 : 0x%02x", regs[2]);

    USBLLDEBUG(" USB debug registers:");
    USBLLDEBUG("  dbg0   : 0x%02x", regs[3]);
    USBLLDEBUG("  dbg1   : 0x%02x", regs[4]);
    USBLLDEBUG("  dbg2   : 0x%02x", regs[5]);
    USBLLDEBUG(" USB counters:");
    USBLLDEBUG("  ack    : 0x%02x", regs[6]);
    USBLLDEBUG("  nack   : 0x%02x", regs[7]);
    USBLLDEBUG("  babble : 0x%02x", regs[8]);
    USBLLDEBUG("  stall  : 0x%02x", regs[9]);
    USBLLDEBUG("  crcerr : 0x%02x", regs[10]);
    USBLLDEBUG("  timeout: 0x%02x", regs[11]);
    USBLLDEBUG("  errpid : 0x%02x", regs[12]);
    USBLLDEBUG("  cplt   : 0x%02x", regs[13]);

    for (i=0;i<MAX_USB_CHANNELS;i++) {
        fpga__read_usb_block(USB_REG_CHAN_CONF1(i), regs, 11);
        if ((regs[2]&0x80)==0) {
            USBLLDEBUG("  Channel %d: Disabled", i);
        } else {
            USBLLDEBUG("  Channel %d:", i);
            USBLLDEBUG("    Eptype    : %d", (regs[0]>>6));
            USBLLDEBUG("    Maxsize   : %d", (regs[0] & 0x1F));
            USBLLDEBUG("    Ep Num    : %d", (regs[1] & 0x0F));
            USBLLDEBUG("    Address   : %d", (regs[2] & 0x7F));
            USBLLDEBUG("    IntConf   : %02x", (regs[3]));
            USBLLDEBUG("    IntStat   : %02x", (regs[4]));
            USBLLDEBUG("    Interval  : %d", (regs[5]));
            USBLLDEBUG("      T Dpid  : %d", (regs[8] & 3));
            USBLLDEBUG("      T Seq   : %d", ((regs[8] >> 2) & 1));
            USBLLDEBUG("      T Retr  : %d", ((regs[8] >> 5) & 3));
            USBLLDEBUG("      T Size  : %d", ((regs[10] & 0x7f)));
            USBLLDEBUG("      T Cnt   : %d", ((regs[10] >> 7)));
        }
    }
    USBLLDEBUG("  IntLine  : %d",  gpio_get_level(PIN_NUM_USB_INTERRUPT));
}
