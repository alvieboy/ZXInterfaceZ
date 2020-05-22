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


static TimerHandle_t enum_timer;

struct chconf
{
    int (*completion)(uint8_t channel, uint8_t status, void *userdata);
    void *userdata;
    uint16_t memaddr;
};

#define MAX_USB_CHANNELS 8

static struct chconf channel_conf[MAX_USB_CHANNELS] = {0};
static uint8_t channel_alloc_bitmap = 0; // Channel 0 reserved


enum usb_state {
    WAIT_RESETCOMPLETE,
    WAIT_SETUP_DEVICEDESCRIPTOR,
    WAIT_IN_DEVICE_DESCRIPTOR1,
    WAIT_SETUP_ADDRESS,
    WAIT_IN_ADDRESS,
    ADDRESSED,
    WAIT_SETUP_USER,
    WAIT_IN_USER
};

static enum usb_state usb_state;

static uint8_t usb_address = 0;

static struct usb_device_info new_device;


static int usb_ll__setup_completed(uint8_t channel, uint8_t intstatus, void*userdata);


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
    uint8_t clr = intpend;

    struct chconf *c = &channel_conf[channel];

    fpga__write_usb(USB_REG_CHAN_INTCLR(channel), clr);

    c->completion( channel, clr, c->userdata );
}

void usb_ll__interrupt()
{
    uint8_t regs[4];
    ESP_LOGI(TAG, "USB interrupt");
    if (usb_ll__read_status(regs)==0) {
        ESP_LOGI(TAG," Status regs %02x %02x %02x %02x",
                 regs[0],
                 regs[1],
                 regs[2],
                 regs[3]);
    }

    if (regs[1] & USB_INTPEND_CONN) {
        ESP_LOGI(TAG,"USB connection event");
        // Send reset.
        fpga__write_usb(USB_REG_STATUS, (USB_STATUS_PWRON |USB_STATUS_RESET));
        // Wait for 10ms, then start enumeration.
        xTimerStart(enum_timer, 0);
    }

    if (regs[1] & USB_INTPEND_DISC) {
        //ESP_LOGI(TAG,"USB disconnection event");
    }

    if (regs[1] & USB_INTPEND_OVERCURRENT) {
        ESP_LOGI(TAG,"USB overcurrent event");
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


uint16_t usb_ll__base_address_for_channel(uint8_t chan)
{
    return 0;
}


int usb_ll__submit_request(uint8_t channel, usb_dpid_t pid, uint8_t seq, uint8_t *data, uint8_t datalen,
                          int (*reap)(uint8_t channel, uint8_t status, void*), void*userdata)
{
    uint8_t regconf[3];
    uint16_t epmemaddress = usb_ll__base_address_for_channel(channel);

    // Write epmem data
    if (data!=NULL) {
        fpga__write_usb_block(USB_REG_DATA(epmemaddress), data, datalen);
    }

    regconf[0] = (uint8_t)pid | ((seq&1)<<2) | ((epmemaddress>>8)<<3);
    regconf[1] = epmemaddress & 0xFF;
    regconf[2] = 0x80 | (datalen & 0x7F);

    dump__buffer(regconf, 3);

    channel_conf[channel].completion = reap;
    channel_conf[channel].userdata = userdata;

    fpga__write_usb_block(USB_REG_CHAN_TRANS1(channel), regconf, 3);

    return 0;
}

static uint8_t usb_ll__allocate_address()
{
    usb_address++;
    if (usb_address>127) {
        usb_address = 1;
    }
    return usb_address;
}

static void usb_ll__device_descriptor1_received()
{
    // Assign address.
    uint8_t address = usb_ll__allocate_address();

    usb_setup_t setup;
    setup.bmRequestType  = USB_REQ_TYPE_STANDARD | USB_HOST_TO_DEVICE;
    setup.bRequest = USB_REQ_SET_ADDRESS;
    setup.wValue = __le16(address);
    setup.wIndex = __le16(0);
    setup.wLength = __le16(0);

    new_device.address = address;

    usb_ll__submit_request(0, PID_SETUP, 0, setup.data, sizeof(setup), &usb_ll__setup_completed, NULL);

    usb_state = WAIT_SETUP_ADDRESS;
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
        fpga__read_usb_block(USB_REG_DATA( channel_conf[channel].memaddr ), target, inlen);
        ESP_LOGI(TAG,"Response from device:");
        dump__buffer(target,inlen);
    }
    *rxlen = inlen;
    return 0;
}

static int usb_ll__in_completed(uint8_t channel, uint8_t intstatus, void*userdata)
{
    uint8_t indata[64];
    uint8_t inlen;
    uint8_t trans_size = 0;

    if (intstatus & 1) {
        usb_ll__read_in_block(channel, indata, &trans_size);
    }

    switch (usb_state) {
    case WAIT_IN_DEVICE_DESCRIPTOR1:
        if (intstatus & 1) {
            // Good.
            memcpy(new_device.device_descriptor, indata, USB_LEN_DEV_DESC);

            usb_ll__device_descriptor1_received();
        } else {
            ESP_LOGE(TAG,"Error requesting device descriptor");
        }
        break;
    case WAIT_IN_ADDRESS:
        if (intstatus &1) {
            if (trans_size!=0) {
                ESP_LOGE(TAG,"Device address: non NULL IN reply!");
            } else {
                usb_state = ADDRESSED;
                usb_ll__device_addressed(&new_device);
            }
        } else {
            ESP_LOGE(TAG,"Error setting device address");
        }
        break;
    case ADDRESSED:
        /* fall-through */
    case WAIT_IN_USER:

        // Send to upper layer
        //usb_ll__in_completed_callback(channel, intstatus);

        //channel_conf[ channel ].in_complete(channel, intstatus, userdata);

        break;
    default:
        ESP_LOGE(TAG,"Invalid state when receiving IN data!!");
        break;
    }
    return 0;
}

static int usb_ll__setup_completed(uint8_t channel, uint8_t intstatus, void*userdata)
{
    if (intstatus&0x01) {

        // Completed.
        // Send IN request
        switch (usb_state) {
        case WAIT_SETUP_DEVICEDESCRIPTOR:
            ESP_LOGI(TAG, "Send IN request");
            usb_ll__submit_request(0, PID_IN, 0, NULL, USB_DEVICE_DESC_SIZE, &usb_ll__in_completed, NULL);
            usb_state = WAIT_IN_DEVICE_DESCRIPTOR1;
            break;

        case WAIT_SETUP_ADDRESS:
            usb_ll__submit_request(0, PID_IN, 0, NULL, 0, &usb_ll__in_completed, NULL);
            usb_state = WAIT_IN_ADDRESS;
            break;
        case WAIT_SETUP_USER:
            //usb_ll__setup_completed_callback(channel, intstatus);
            break;
        default:
            break;

        }
    } else {
        ESP_LOGE(TAG,"Invalid SETUP completed state");
    }
    return 0;
}

static void usb_ll__start_enum( TimerHandle_t xTimer )
{
    usb_setup_t setup;

    setup.bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE | USB_DEVICE_TO_HOST;
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = __le16(USB_DESC_DEVICE);
    setup.wIndex = __le16(0);
    setup.wLength = __le16(USB_LEN_DEV_DESC);
/*    uint8_t req[] = {
        0x80,
        0x06,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x12
    };*/
    ESP_LOGI(TAG, "Send SETUP GET_DESCRIPTOR request");

    usb_state = WAIT_SETUP_DEVICEDESCRIPTOR;

    usb_ll__submit_request(0, PID_SETUP, 0, setup.data, sizeof(setup), &usb_ll__setup_completed, NULL);
}

static void usb_ll__task(void *pvParameters)
{
    ESP_LOGI(TAG, "USB: Low level init");
    // Power on, enable interrupts
    uint8_t regval[4] = {
        (USB_STATUS_PWRON),
        0x00, // Unused,
        (USB_INTCONF_GINTEN | USB_INTCONF_DISC | USB_INTCONF_CONN | USB_INTCONF_OVERCURRENT),
        0xff // Clear pending
    };

    fpga__write_usb_block(USB_REG_STATUS, regval, 4);

    /* Configure EP0 setup channel */
    /*
    regval[0] = 0;
    regval[1] = 0;
    regval[2] = 0;
    fpga__write_usb_block(USB_REG_CHAN_TRANS1(0),regval,3);

    uint8_t epconf[] = {
        0x3F, // EPtype 0, max size 64         // CONF1
        0x20, // -- Ep 0, OUT - FIXME          // CONF2
        0x80, // Address 0. EP enabled.       // CONF3
        0xFF, // All interrupts enabled.       // INTCONF
        0xFF, // All interrupt clear           // INTCLR
    };

    fpga__write_usb_block(USB_REG_CHAN_CONF1(0), epconf, 5);

    */

    usb_ll__alloc_channel(0x00,
                          EP_TYPE_CONTROL,
                          64,
                          0);
    // Set up chan0
    channel_conf[0].memaddr = 0x0000;


    enum_timer =  xTimerCreate("enum",
                               10/portTICK_RATE_MS,
                               pdFALSE,
                               NULL,
                               usb_ll__start_enum
                              );

    while (1) {
        vTaskDelay(5000 / portTICK_RATE_MS);
        uint8_t epconf[8];
        fpga__read_usb_block(USB_REG_CHAN_CONF1(0), epconf, 8);
        dump__buffer(epconf, 8);

    }
}

int usb_ll__init()
{
    xTaskCreate(usb_ll__task, "usb_task", 4096, NULL, 10, NULL);
    return 0;
}




