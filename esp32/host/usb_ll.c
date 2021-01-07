#include "usb_ll.h"
#include "usb_link.h"
#include <libusb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"

struct libusb_context *libusbctx;
struct libusb_device *device;
struct libusb_device_handle *device_handle;

struct epchannel
{
    uint8_t devaddr;
    eptype_t eptype;
    uint8_t maxsize;
    uint8_t epnum;
    void *user;
    uint8_t out_data[512];
    uint8_t in_data[512];
    unsigned out_datalen;
    unsigned in_datalen;
    bool indataready;
};

#define NUM_CHANNELS 4
static struct epchannel channels[NUM_CHANNELS];


static struct libusb_device *usb_ll__open_vid_pid(uint16_t vid, uint16_t pid,
                                                 struct libusb_device_handle**handle);

static int usb_ll_do_open(uint16_t vid, uint16_t pid)
{
    int status;

    device = usb_ll__open_vid_pid(vid, pid, &device_handle);
    if (device) {
        libusb_set_auto_detach_kernel_driver(device_handle, 1);
        status = libusb_claim_interface(device_handle, 0);
    }

    return status;
}

int usb_attach(const char *id)
{
    char *delim = strchr(id,':');
    char *endp = NULL;
    if (!delim) {
        fprintf(stderr,"USB: No delim\n");
        return -1;
    }
    char *vid_alloc = strndup(id, delim-id);
    if (!vid_alloc) {
        fprintf(stderr,"USB: Cannot allocate vid\n");

        return -1;
    }

    int vid = strtoul(vid_alloc, &endp, 16);
    if (!endp || *endp!='\0') {
        fprintf(stderr,"USB: Invalid vid %s\n", vid_alloc);
        return -1;
    }

    delim++;

    int pid = strtoul(delim, &endp, 16);
    if (!endp || *endp!='\0') {
        fprintf(stderr,"USB: Invalid pid %s\n", delim);

        return -1;
    }

    int r =usb_ll_do_open(vid,pid);
    if (r==0) {
        // propagate
        usb__trigger_interrupt();
    }
}

static struct libusb_device *usb_ll__open_vid_pid(uint16_t vid, uint16_t pid,struct libusb_device_handle**handle)
{
    libusb_device *dev, **devs;
    int i, status;
    struct libusb_device_descriptor desc;
    struct usb_device_handle *devhandle;


    if (libusb_get_device_list(libusbctx, &devs) < 0) {
        fprintf(stderr, "USB: Cannot get device list\n");
        return NULL;
    }
    for (i=0; (dev=devs[i]) != NULL; i++) {
        status = libusb_get_device_descriptor(dev, &desc);
        if (status >= 0) {
            if (desc.idVendor == vid && desc.idProduct == pid) {
                status = libusb_open(dev, handle);
                if (status<0) {
                    fprintf(stderr, "USB: Cannot open USB device\n");
                }
                fprintf(stderr, "USB: USB device opened\n");
                libusb_free_device_list(devs, 1);
                return dev;
            }
        }
    }
    fprintf(stderr, "USB: Cannot find USB device vid=%04x pid=%04x\n", vid, pid);
    libusb_free_device_list(devs, 1);

    return NULL;
}


int usb_ll__init(void)
{
    int i;
    for (i=0;i<4;i++) {
        channels[i].devaddr = 0xff;
    }

    loglevel |= (DEBUG_ZONE_USBLL|DEBUG_ZONE_USBH);
    libusb_init(&libusbctx);
    libusb_set_option(libusbctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    // trigger interrupt after 1 sec.
    return 0;
}

void usb_ll__set_power(int on)
{
}

void usb_ll__dump_info()
{
}

void usb_ll__channel_set_interval(uint8_t chan, uint8_t interval)
{
}


struct usb_ll_internal_callback
{
    int (*reap)(uint8_t channel, uint8_t status, void*);
    void*reapdata;
    uint8_t channel;
};

static void usb_ll__libusb_transfer_callback(struct libusb_transfer *transfer)
{
    void *userdata = transfer->user_data;
//    fprintf(stderr,"USB: Request completed, actual_length=%d\n", transfer->actual_length);
    // Save data into channel

    struct usb_ll_internal_callback *cbd = (struct usb_ll_internal_callback*)userdata;

    struct epchannel *epc = &channels[cbd->channel];
    int status = 0x00;




    switch (transfer->status) {
    case LIBUSB_TRANSFER_COMPLETED:
        switch (transfer->type) {
        case LIBUSB_TRANSFER_TYPE_CONTROL:
        case LIBUSB_TRANSFER_TYPE_BULK:
        case LIBUSB_TRANSFER_TYPE_INTERRUPT:
            if (transfer->actual_length>0) {

                memcpy(epc->in_data, libusb_control_transfer_get_data(transfer), transfer->actual_length);
                epc->in_datalen = transfer->actual_length;
            }

        }
        status = 0x01;
        break;
    default:
        break;
    }

    libusb_free_transfer(transfer);

    cbd->reap( cbd->channel, status, cbd->reapdata);

}

int usb_ll__submit_request(uint8_t channel, uint16_t epmemaddr,
                           usb_dpid_t pid, uint8_t seq, uint8_t *data, uint8_t datalen,
                           int (*reap)(uint8_t channel, uint8_t status,void*), void*reapdata)
{
    struct libusb_transfer *t = libusb_alloc_transfer(0);

    if (!t)
        return -1;

    struct usb_ll_internal_callback *localcallback = malloc(sizeof(struct usb_ll_internal_callback));

    localcallback->channel =  channel;
    localcallback->reap = reap;
    localcallback->reapdata = reapdata;

    struct epchannel *epc = &channels[channel];
    int r;

    memcpy( epc->out_data, data, datalen);

    epc->out_datalen = datalen;

    switch (pid) {
    case PID_IN: /* Fall-through */
    case PID_OUT:
        libusb_fill_bulk_transfer(t,
                                  device_handle,
                                  epc->epnum,
                                  epc->out_data,
                                  datalen,
                                  &usb_ll__libusb_transfer_callback,
                                  localcallback,
                                  1000);
        break;
    case PID_SETUP:
        epc->indataready = 0;
        libusb_fill_control_transfer(t,
                                     device_handle,
                                     epc->out_data,
                                     &usb_ll__libusb_transfer_callback,
                                     localcallback,
                                     1000);
    };

    epc->in_datalen = 0;

    //fprintf(stderr,"USB: Submitting request type %d\n", pid);
    r = libusb_submit_transfer(t);
    //fprintf(stderr,"USB: Submitted request type %d: %d\n",pid, r);
    return r;
}

int usb_ll__alloc_channel(uint8_t devaddr,
                          eptype_t eptype,
                          uint8_t maxsize,
                          uint8_t epnum,
                          void *userdata)
{
    int i;
    for (i=0;i<NUM_CHANNELS;i++) {
        if (channels[i].devaddr ==0xff) {
            channels[i].devaddr = devaddr;
            channels[i].eptype = eptype;
            channels[i].maxsize = maxsize;
            channels[i].epnum = epnum;
            channels[i].user = userdata;
            channels[i].indataready = false;
            return i;
        }
    }
    return -1;
}

int usb_ll__release_channel(uint8_t channel)
{
    channels[channel].devaddr = 0xff;
}

int usb_ll__read_in_block(uint8_t channel, uint8_t *target, uint8_t *rxlen)
{
    struct epchannel *epc = &channels[channel];
    if (epc->in_datalen) {
        memcpy(target, epc->in_data, epc->in_datalen);
        *rxlen = epc->in_datalen;
    }
    else
        *rxlen = 0;

    return 0;
}


void usb_ll__reset(void)
{
    if (device_handle) {
        printf("USB reset device\n");
        libusb_reset_device(device_handle);
        printf("USB reset device done\n");
    }
}

void usb_ll__idle()
{
    libusb_handle_events(libusbctx);
}

uint8_t usb_ll__get_address(void)
{
    return libusb_get_device_address (device);
}

#if 1
void usb_ll__interrupt()
{
    usb_ll__connected_callback();
}
#endif
