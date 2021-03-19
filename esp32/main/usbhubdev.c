#include "usb_driver.h"
#include "usb_device.h"
#include "usb_defs.h"
#include "usbh.h"
#include "usb_descriptor.h"
#include "log.h"
#include "usbhub.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define TAG "USBHUBDEV"
#define INTERFACE_CLASS_HUB 0x09

#define USB_DESC_TYPE_HUB 0x29
#define USB_DESC_HUB                       ((USB_DESC_TYPE_HUB << 8) & 0xFF00)

#define USB_LEN_HUB_DESC 71

#define USB_RT_PORT       (USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_OTHER)
#define USB_RT_HUB	  (USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_DEVICE)

#define HUB_PORT_STATUS		0
#define HUB_PORT_PD_STATUS	1
#define HUB_EXT_PORT_STATUS	2

#define USB_PORT_STAT_CONNECTION	0x0001
#define USB_PORT_STAT_ENABLE		0x0002
#define USB_PORT_STAT_SUSPEND		0x0004
#define USB_PORT_STAT_OVERCURRENT	0x0008
#define USB_PORT_STAT_RESET		0x0010
#define USB_PORT_STAT_L1		0x0020
#define USB_PORT_STAT_POWER		0x0100
#define USB_PORT_STAT_LOW_SPEED		0x0200
#define USB_PORT_STAT_HIGH_SPEED        0x0400
#define USB_PORT_STAT_TEST              0x0800
#define USB_PORT_STAT_INDICATOR         0x1000

#define USB_PORT_STAT_C_CONNECTION	0x0001
#define USB_PORT_STAT_C_ENABLE		0x0002
#define USB_PORT_STAT_C_SUSPEND		0x0004
#define USB_PORT_STAT_C_OVERCURRENT	0x0008
#define USB_PORT_STAT_C_RESET		0x0010
#define USB_PORT_STAT_C_L1		0x0020

#define USB_PORT_FEAT_CONNECTION	0
#define USB_PORT_FEAT_ENABLE		1
#define USB_PORT_FEAT_SUSPEND		2
#define USB_PORT_FEAT_OVER_CURRENT	3
#define USB_PORT_FEAT_RESET		4
#define USB_PORT_FEAT_L1		5
#define USB_PORT_FEAT_POWER		8
#define USB_PORT_FEAT_LOWSPEED		9

#define USB_PORT_FEAT_C_CONNECTION	16
#define USB_PORT_FEAT_C_ENABLE		17
#define USB_PORT_FEAT_C_SUSPEND		18
#define USB_PORT_FEAT_C_OVER_CURRENT	19
#define USB_PORT_FEAT_C_RESET		20

#define USB_PORT_FEAT_TEST              21
#define USB_PORT_FEAT_INDICATOR         22
#define USB_PORT_FEAT_C_PORT_L1         23

// Should be about 1sec
#define HUB_PROBE_INTERVAL_US 10000000

typedef struct {
    uint8_t  bDescriptorLength;
    uint8_t  bDescriptorType;
    uint8_t  bNumberOfPorts;
    le_uint16_t wHubCharacteristics;
    uint8_t  bPowerOnToPowerGood;
    uint8_t  bHubControlCurrent;
    uint8_t  bRemoveAndPowerMask[8];
} __attribute__((packed)) usb_hub_descriptor_t;

typedef struct {
    le_uint16_t wPortStatus;
    le_uint16_t wPortChange;
    le_uint32_t dwExtPortStatus;
} __attribute__ ((packed)) usb_port_status_t;


struct usb_hub_dev
{
    struct usb_hub hub;
    struct usb_device *dev;
    struct usb_interface *intf;
    usb_hub_descriptor_t hubdesc;
    usb_port_status_t port_status __attribute__((aligned(4))); // To avoid SPI DMA allocations
#ifndef USBHUB_USE_INTERRUPT
    esp_timer_handle_t portchangetimer;
#endif
};

static int usbhubdev__set_power(struct usb_hub_dev *self, int port, int poweron);
static int usbhubdev__check_ports(struct usb_hub_dev *self);
static int usbhubdev__set_port_feature(struct usb_hub_dev *self, int port, int feature);
static int usbhubdev__clear_port_feature(struct usb_hub_dev *self, int port, int feature);
static int usbhubdev__port_status(struct usb_hub_dev *self, int port, uint16_t *status, uint16_t *change);
static void usbhubdev__timer_elapsed(void*data);

static int usbhubdev__reset(struct usb_hub_dev *self, int port, uint16_t *status)
{
    uint16_t change = 0;

    int r = usbhubdev__set_port_feature(self, port, USB_PORT_FEAT_RESET);

    if (r<0)
        return r;

    vTaskDelay(40 / portTICK_RATE_MS);

    do {
        r = usbhubdev__port_status(self, port, status, &change);

        if (r<0)
            return r;

        if (change & USB_PORT_STAT_C_RESET) {
            r = usbhubdev__clear_port_feature(self, port, USB_PORT_FEAT_C_RESET);
            break;
        }
    } while (1);

    return r;
}

int usbhubdev__hub_reset(struct usb_hub *h,int port)
{
    struct usb_hub_dev *self = (struct usb_hub_dev*)h;
    uint16_t status = 0;


    return usbhubdev__reset(self, port, &status);
}

int usbhubdev__hub_get_ports(struct usb_hub *h)
{
    struct usb_hub_dev *self = (struct usb_hub_dev*)h;
    return self->hubdesc.bNumberOfPorts;
}

int usbhubdev__hub_set_power(struct usb_hub *h, int port, int power)
{
    struct usb_hub_dev *self = (struct usb_hub_dev*)h;
    return usbhubdev__set_power(self, port, power);
}

int usbhubdev__init(struct usb_hub *h)
{
    //struct usb_hub_dev *self = (struct usb_hub_dev*)h;

    //return usbhubdev__check_ports(self);
    return 0;
}

static int usbhubdev__probe(struct usb_device *dev, struct usb_interface *i)
{
    usb_interface_descriptor_t *intf = i->descriptors[0];
    int intf_len = i->descriptorlen[0];
    int r = -1;
    unsigned short size_transferred;

    struct usb_hub_dev *self;

    ESP_LOGI(TAG,"usb_hub_dev: probing device %d %d %d",
             intf->bInterfaceClass,
             intf->bInterfaceSubClass,
             intf->bInterfaceProtocol
            );

    if (intf->bInterfaceClass != INTERFACE_CLASS_HUB) {
        return r;
    }

    if (intf->bNumEndpoints!=1) {
        ESP_LOGE(TAG, "usb_hub_dev: too many endpoints");
        return -1;
    }


    if (usbh__set_configuration(dev, dev->config_descriptor->bConfigurationValue)<0)
        return -1;

    self = malloc(sizeof(struct usb_hub_dev));

    if (self==NULL) {
        ESP_LOGE(TAG, "usb_hub_dev: no memory");
        return -1;
    }

    self->dev = dev;
    self->intf = i;

    self->hub.init      = &usbhubdev__init;
    self->hub.reset     = &usbhubdev__hub_reset;
    self->hub.get_ports = &usbhubdev__hub_get_ports;
    self->hub.set_power = &usbhubdev__hub_set_power;

    if (usbh__get_descriptor(dev,
                             USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_DEVICE,
                             USB_DESC_HUB,
                             (uint8_t*)&self->hubdesc,
                             USB_LEN_HUB_DESC,
                             &size_transferred)<0) {
        ESP_LOGE(TAG,"Cannot read USB HUB descriptor");
        return -1;
    }

    if (size_transferred < 8) {
        ESP_LOGE(TAG,"USB HUB descriptor too short");
        free(self);
        return -1;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = &usbhubdev__timer_elapsed,
        .arg = self,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "usbhubtimer"
    };

    r = esp_timer_create(&timer_args, &self->portchangetimer);

    if (r<0) {
        ESP_LOGE(TAG,"Cannot create timer");
        free(self);
        return -1;
    }

    r = esp_timer_start_periodic(self->portchangetimer, HUB_PROBE_INTERVAL_US);
    if (r<0) {
        ESP_LOGE(TAG,"Cannot start timer");
        esp_timer_delete(self->portchangetimer);
        return r;
    }


    i->drvdata = self;

    
    dev->self_hub = &self->hub;

    usbhub__new(&self->hub);


    return 0;
}


static int usbhubdev__clear_hub_feature(struct usb_hub_dev *self, int feature)
{
    return usbh__control_msg(self->dev,
                             USB_REQ_CLEAR_FEATURE,
                             USB_RT_HUB,
                             feature,
                             0,
                             NULL,
                             0,
                             1000,
                             NULL);
}

static int usbhubdev__clear_port_feature(struct usb_hub_dev *self, int port, int feature)
{
    return usbh__control_msg(self->dev,
                             USB_REQ_CLEAR_FEATURE,
                             USB_RT_PORT,
                             feature,
                             port,
                             NULL,
                             0,
                             1000,
                             NULL);
}

static int usbhubdev__set_port_feature(struct usb_hub_dev *self, int port, int feature)
{
    return usbh__control_msg(self->dev,
                             USB_REQ_SET_FEATURE,
                             USB_RT_PORT,
                             feature,
                             port,
                             NULL,
                             0,
                             1000,
                             NULL);
}




static int usbhubdev__set_power(struct usb_hub_dev *self, int port, int poweron)
{
    int r;
    if (poweron) {
        r = usbhubdev__set_port_feature(self, port, USB_PORT_FEAT_POWER);
    } else {
        r = usbhubdev__clear_port_feature(self, port, USB_PORT_FEAT_POWER);
    }
    return r;
}
           /*
static int usbhubdev__get_hub_status(struct usb_device hdev,
                                     struct usb_hub_status *data)
{
    return usbh__control_msg(dev,
                           USB_REQ_GET_STATUS,
                           REQ_DEVICE_TO_HOST | USB_RT_HUB, 0, 0,
                           data, sizeof(*data), 1000);
}
*/

static int usbhubdev__get_port_status(struct usb_device *dev,
                                      int port,
                                      void *data,
                                      uint16_t value,
                                      uint16_t length);


static int usbhubdev__port_status(struct usb_hub_dev *self, int port, uint16_t *status, uint16_t *change)
{
    int r = usbhubdev__get_port_status(self->dev,
                                       port, &self->port_status, HUB_PORT_STATUS, 4);

    if (r<0)
        return -1;

    *status = __le16(self->port_status.wPortStatus);
    *change = __le16(self->port_status.wPortChange);

    return r;
}

static int usbhubdev__enumerate_port(struct usb_hub_dev *self, int port)
{
    uint16_t status = 0;
    usb_speed_t speed = USB_FULL_SPEED;

    int r = usbhubdev__reset(self, port, &status);
    if (r<0)
        return r;

    ESP_LOGI(TAG, "Device status: %04x\n", status);

    if ( (status & USB_PORT_STAT_ENABLE) &&
        (status & USB_PORT_STAT_CONNECTION)) {

        if (status & USB_PORT_STAT_HIGH_SPEED) {
            ESP_LOGE(TAG,"High-speed device, cannot handle!!!");
            return -1;
        }
        if (status & USB_PORT_STAT_LOW_SPEED) {
            speed = USB_LOW_SPEED;
        }
        usbh__hub_port_connected(&self->hub, port, speed);
    }
    return r;
}

static void usbhubdev__port_disconnect(struct usb_hub_dev *self, int port)
{
    usbhub__port_disconnect(&self->hub, port);
}

static int usbhubdev__check_ports(struct usb_hub_dev *self)
{
    unsigned port;
    uint32_t enumerate = 0;
    uint32_t disconnect = 0;

    for (port=1; port<=self->hubdesc.bNumberOfPorts; port++)
    {
        uint16_t portstatus = 0, portchange = 0;

        if (usbhubdev__port_status(self, port, &portstatus, &portchange)<0)
            return -1;

        if (portchange & USB_PORT_STAT_C_CONNECTION) {
            usbhubdev__clear_port_feature(self, port, USB_PORT_FEAT_C_CONNECTION);
            if (portstatus & USB_PORT_STAT_CONNECTION)
                enumerate |= (1<<(port-1));
            else
                disconnect |= (1<<(port-1));
        }

        if (portchange & USB_PORT_STAT_C_ENABLE) {
            usbhubdev__clear_port_feature(self, port, USB_PORT_FEAT_C_ENABLE);
        }

        if (portchange & USB_PORT_STAT_C_RESET) {
            usbhubdev__clear_port_feature(self, port, USB_PORT_FEAT_C_RESET);
        }

        if (portchange & USB_PORT_STAT_C_OVERCURRENT) {
            usbhubdev__clear_port_feature(self, port, USB_PORT_FEAT_C_OVER_CURRENT);
            usbhub__overcurrent(&self->hub, port);
        }
        //ESP_LOGI(TAG,"Port %d, status 0x%04x change %04x", port, portstatus, portchange);
    }

    if (enumerate) {
        for (port=0;port<self->hubdesc.bNumberOfPorts;port++) {
            if (enumerate & (1<<port)) {
                usbhubdev__enumerate_port(self, port+1);
            }
        }
    }

    if (disconnect) {
        for (port=0;port<self->hubdesc.bNumberOfPorts;port++) {
            if (disconnect & (1<<port)) {
                usbhubdev__port_disconnect(self, port+1);
            }
        }
    }

    return 0;
}

static int usbhubdev__get_port_status(struct usb_device *dev, int port, void *data, uint16_t value, uint16_t length)
{
    ESP_LOGD(TAG, "Get port %d status", port);
    return usbh__control_msg(dev,
                             USB_REQ_GET_STATUS,
                             USB_DEVICE_TO_HOST | USB_RT_PORT,
                             value,
                             port,
                             data,
                             length,
                             1000,
                             NULL);
}

#ifndef USBHUB_USE_INTERRUPT
static void usbhubdev__timer_elapsed(void*data)
{
    struct usb_hub_dev *self =(struct usb_hub_dev*)data;

    usbhubdev__check_ports(self);
}
#endif

void usbhubdev__disconnect(struct usb_device *dev, struct usb_interface *intf)
{
    struct usb_hub_dev *self = (struct usb_hub_dev*)intf->drvdata;

    ESP_LOGI(TAG,"HUB disconnected");
#ifndef USBHUB_USE_INTERRUPT
    esp_timer_stop(self->portchangetimer);

    esp_timer_delete(self->portchangetimer);
#endif

    for (int port=0;port<self->hubdesc.bNumberOfPorts;port++) {
        struct usb_device *dev = usbdevice__find_by_hub_port(&self->hub, port);
        if (dev) {
            usbdevice__disconnect(dev);
        }
    }


}

const struct usb_driver usb_hub_driver = {
    .probe = &usbhubdev__probe,
    .disconnect = &usbhubdev__disconnect
};

