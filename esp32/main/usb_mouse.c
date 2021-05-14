/*

 USB Mouse (boot protocol) driver.

 */

#include "usb_driver.h"
#include "usb_defs.h"
#include "usbh.h"
#include <stdlib.h>
#include "usb_descriptor.h"
#include "usb_mouse.h"
#include "esp_log.h"
#include "defs.h"

#define TAG "USBMOUSE"

#define DEBUG(x...)

#define INTERFACE_CLASS_HID 0x03
#define INTERFACE_SUBCLASS_BOOT_INTERFACE 0x01
#define INTERFACE_PROTOCOL_MOUSE 0x02

struct usb_mouse {
    uint8_t epchan:7;
    uint8_t seq:1;
    struct usb_device *dev;
    struct usb_interface *intf;
    uint32_t payload[0];
};

static int usb_mouse__submit_in(struct usb_mouse *m);

static int usb_mouse__transfer_completed(uint8_t channel, uint8_t status, void*userdata)
{
    struct usb_mouse *m = (struct usb_mouse*)userdata;
    uint8_t rxlen = 8;

    if (status&0x01) {
        ESP_LOGI(TAG, "usb_mouse: got data stat %d", status);
        // Fetch data from ....
        usb_ll__read_in_block(m->epchan, m->payload, &rxlen);


#if 0
        input_report_key(dev, BTN_LEFT,   data[0] & 0x01);
	input_report_key(dev, BTN_RIGHT,  data[0] & 0x02);
	input_report_key(dev, BTN_MIDDLE, data[0] & 0x04);
	input_report_key(dev, BTN_SIDE,   data[0] & 0x08);
	input_report_key(dev, BTN_EXTRA,  data[0] & 0x10);

	input_report_rel(dev, REL_X,     data[1]);
	input_report_rel(dev, REL_Y,     data[2]);
        input_report_rel(dev, REL_WHEEL, data[3]);
#endif
        m->seq = !m->seq;
    } else {
        ESP_LOGI(TAG, "usb_mouse: error %d, resubmitting", status);
    }

    usb_mouse__submit_in(m);

    return 0;
}

static int usb_mouse__submit_in(struct usb_mouse *m)
{
    return usb_ll__submit_request(m->epchan,
                                  PID_IN,
                                  (uint8_t*)&m->payload[0],
                                  8, // Report descriptor size!!!
                                  usb_mouse__transfer_completed,
                                  m);
}

static int usb_mouse__probe(struct usb_device *dev, struct usb_interface *i)
{
    usb_interface_descriptor_t *intf = i->descriptors[0];
    int intf_len = i->descriptorlen[0];
    int r = -1;

    ESP_LOGI(TAG,"usb_mouse: probing device %d %d %d",
             intf->bInterfaceClass,
             intf->bInterfaceSubClass,
             intf->bInterfaceProtocol
            );

    if ((intf->bInterfaceClass != INTERFACE_CLASS_HID) ||
        (intf->bInterfaceSubClass != INTERFACE_SUBCLASS_BOOT_INTERFACE) ||
        (intf->bInterfaceProtocol != INTERFACE_PROTOCOL_MOUSE))
    {
        ESP_LOGI(TAG, "usb_mouse: not a mouse");
        return r;
    }

    if (intf->bNumEndpoints!=1) {
        ESP_LOGE(TAG, "usb_mouse: too many endpoints");
        return -1;
    }

    int index = 0;
    usb_endpoint_descriptor_t *ep;

    struct usb_mouse *m = malloc(sizeof(struct usb_mouse) + 8); // report size,

    if (m==NULL) {
        return -1;
    }

    m->dev = dev;
    m->intf = i;
    m->seq = 0;

    do {
        ep = (usb_endpoint_descriptor_t*)usb__find_descriptor( intf, intf_len, USB_DESC_TYPE_ENDPOINT, index);
        if (ep) {
            ESP_LOGI(TAG, "usb_mouse: found endpoint %d",ep->bEndpointAddress);
            break;
        }
        index++;
    } while (ep);

    if (!ep) {
        ESP_LOGE(TAG,"usb_mouse: cannot find interrupt endpoint");
        free(m);
    } else {
        if (ep->bEndpointAddress & 0x80) {
            // In endpoint
            if (ep->bmAttributes == 0x03) {
                // Allocate channel
                m->epchan = usb_ll__alloc_channel(dev->address,
                                                  0x03, // Interrupt endpoint
                                                  ep->wMaxPacketSize,
                                                  ep->bEndpointAddress,
                                                  dev->fullspeed,
                                                  dev
                                                 );
                ESP_LOGI(TAG, "usb_mouse: setting interval");
                usb_ll__channel_set_interval(m->epchan, ep->bInterval);

                ESP_LOGI(TAG, "usb_mouse: setting configuration");
                usbh__set_configuration(dev, dev->config_descriptor->bConfigurationValue);

                // Start IN requests
                ESP_LOGI(TAG, "usb_mouse: Submitting IN requests");

                usb_mouse__submit_in(m);

                i->drvdata = m;

                r=0;
            }
        }
    }

    return r;
}

static void usb_mouse__disconnect(struct usb_device *dev, struct usb_interface *intf)
{
    struct usb_mouse *m = intf->drvdata;
    // Cancel any pending requests
    usb_ll__release_channel(m->epchan);
    free(m);
    intf->drvdata = NULL;
}

const struct usb_driver usb_mouse_driver = {
    .probe = &usb_mouse__probe,
    .disconnect = &usb_mouse__disconnect
};
