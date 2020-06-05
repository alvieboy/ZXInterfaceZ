#include "usb_driver.h"
#include "usb_defs.h"
#include "usbh.h"
#include <stdlib.h>
#include "usb_descriptor.h"
#include "usb_hid.h"
#include "esp_log.h"
#include <stdbool.h>
#include "dump.h"
#include "hid.h"
#include <string.h>

#define HIDTAG "HID"
#define DEBUG(x...) ESP_LOGI(HIDTAG,x)

static int usb_hid__set_idle(struct usb_device *dev, usb_interface_descriptor_t *intf);

struct usb_hid
{
    uint8_t epchan:7;
    uint8_t seq:1;
    struct usb_device *dev;
    struct usb_interface *intf;
    struct hid *hid;
    uint8_t *report_desc;
    uint8_t payload[64];
    uint8_t prev_payload[64];
    uint8_t prev_valid;
};

static int usb_hid__submit_in(struct usb_hid *h);

static void usb_hid__parse_report(struct usb_hid *hid)
{
    uint8_t new_value;
    uint8_t old_value;
    bool propagate = false;

    if (!hid->hid)
        return;

    struct hid *h = hid->hid;
    unsigned entry_index = 0;

    if (h->reports == NULL) {
        return;
    } else {
        hid_report_t *rep = h->reports;
        hid_field_t *field = rep->fields;

        ESP_LOGI(HIDTAG," Field ");
        while (field) {
            int i;
            ESP_LOGI(HIDTAG," > start %d len %d", field->report_offset, field->report_size);
            for (i=0;i<field->report_count;i++) {

                ESP_LOGI(HIDTAG," >> index %d", i);

                if (hid_extract_field_aligned(field, i, hid->payload, &new_value)<0)
                    return;
                if (hid->prev_valid) {
                    if (hid_extract_field_aligned(field, i, hid->prev_payload, &old_value)<0)
                        return;
                }
                // Compare
                if (hid->prev_valid) {
                    if (new_value!=old_value)
                        propagate = true;
                } else {
                    propagate = true;
                }
                if (propagate) {
                    ESP_LOGI(HIDTAG,"Field changed");
                    hid__field_entry_changed_callback(usbh__get_device_id(hid->dev), field, entry_index, new_value);
                }
                entry_index++;
            }
            field = field->next;
        }
    }
}

static int usb_hid__transfer_completed(uint8_t channel, uint8_t status, void*userdata)
{
    struct usb_hid *h = (struct usb_hid*)userdata;
    uint8_t rxlen = 64;
    if (!h) {
        ESP_LOGE(HIDTAG, "Invalid private pointer!!!");
        return -1;
    }
    if (status&0x01) {

        ESP_LOGI(HIDTAG, "got data stat %d", status);
        // Fetch data from ....
        usb_ll__read_in_block(h->epchan, h->payload, &rxlen);
        ESP_LOGI(HIDTAG, "Parsing report");
        usb_hid__parse_report(h);

        ESP_LOGI(HIDTAG, "Copying data");
        h->seq = !h->seq;
        memcpy(h->prev_payload, h->payload, rxlen);
        h->prev_valid = 1;
    } else {
        ESP_LOGI(HIDTAG, "error %d, resubmitting", status);
    }

    usb_hid__submit_in(h);

    return 0;
}

static int usb_hid__submit_in(struct usb_hid *m)
{
    return usb_ll__submit_request(m->epchan,
                                  0x0080, // TBD
                                  PID_IN,
                                  m->seq,      // TODO: Check seq.
                                  &m->payload[0],
                                  64, // Report descriptor size!!!
                                  usb_hid__transfer_completed,
                                  m);
}

/*
 0: 7F
 8: 7F
 16: 7F
 24: 7F
 32: FF
 40: 0F
 -- Hat: 0xF
 -- Buttons: 0x0
 48: 00 -- buttons 0
 56: 00 ???

 Report 0 size 56
 > Field offset 0 size 8 count 5
  [0] usage 0x0132 (Z axis)
  [1] usage 0x0135 (Z rotation)
  [2] usage 0x0130 (X axis)
  [3] usage 0x0131 (Y axis)
  [4] usage 0x0100 (Undefined)
 > Field offset 40 size 4 count 1
  [0] usage 0x0139 (Hat switch)
 > Field offset 44 size 1 count 12
  [0] usage 0x0901 (Button 1)
  [1] usage 0x0902 (Button 2)
  [2] usage 0x0903 (Button 3)
  [3] usage 0x0904 (Button 4)
  [4] usage 0x0905 (Button 5)
  [5] usage 0x0906 (Button 6)
  [6] usage 0x0907 (Button 7)
  [7] usage 0x0908 (Button 8)
  [8] usage 0x0909 (Button 9)
  [9] usage 0x090a (Button 10)
  [10] usage 0x090b (Button 11)
  [11] usage 0x090c (Button 12)

 */

static int usb_hid__probe(struct usb_device *dev, struct usb_interface *i)
{
    usb_interface_descriptor_t *intf = i->descriptors[0];
    int intf_len = i->descriptorlen[0];
    bool valid = false;

    DEBUG("     ***** PROBING INTERFACE %d altSetting %d **** class=%02x\n",
          intf->bInterfaceNumber,
          intf->bAlternateSetting,
          intf->bInterfaceClass);

    if (intf->bInterfaceClass == 0x03) {
        if (intf->bInterfaceSubClass == 0x00) { // Not boot
            if (intf->bInterfaceProtocol == 0x00) { // Not mouse/keyboard
                valid = true;
            }
        }
    }

    if (!valid)
        return false;

    usb_hid_descriptor_t *hidd = (usb_hid_descriptor_t*)usb__find_descriptor( intf, intf_len, USB_DESC_TYPE_HID, 0);

    if (!hidd) {
        ESP_LOGE(HIDTAG, "Cannot find HID descriptor");
        return -1;
    }

    DEBUG("HID %p\n", hidd);

    int index = 0;
    usb_endpoint_descriptor_t *ep = NULL;
    do {
        ep = (usb_endpoint_descriptor_t*)usb__find_descriptor( intf, intf_len, USB_DESC_TYPE_ENDPOINT, index);
        if (ep) {
            break;
        }
        index++;
    } while (ep);

    if (!ep) {
        ESP_LOGE(HIDTAG, "Cannot find HID endpoint descriptor");
        return -1;

    }

    if (hidd->bReportDescriptorType != USB_DESC_TYPE_HID_REPORT) {
        ESP_LOGE(HIDTAG, "Cannot handle HID descriptor type 0x%02x", hidd->bDescriptorType);
        return -1;
    }

    if (usbh__set_configuration(dev, dev->config_descriptor->bConfigurationValue)<0)
        return -1;

    if (usb_hid__set_idle(dev, intf)<0)
        return -1;

    // Get report descriptor

    unsigned report_desc_len = __le16(hidd->wItemLength);

    uint8_t *report_desc = malloc (report_desc_len);
    if (NULL==report_desc) {
        ESP_LOGE(HIDTAG,"Cannot allocate memory");
        return -1;
    }

    int r = usbh__get_descriptor(dev,
                                 USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_INTERFACE,
                                 USB_DESC_HID_REPORT | intf->bInterfaceNumber ,
                                 report_desc,
                                 report_desc_len);
    if (r<0) {
        ESP_LOGE(HIDTAG, "Cannot read HID report descriptor");
        free( report_desc );
        return -1;
    }

    ESP_LOGE(HIDTAG, "HID report descriptor len %d", report_desc_len);
    dump__buffer(report_desc, report_desc_len);

    struct hid *hid = hid_parse(report_desc, report_desc_len);

    if (NULL==hid) {
    }

    struct usb_hid *h =  malloc(sizeof(struct usb_hid)); // TBD.

    h->dev = dev;
    h->intf = i;
    h->seq = 0;
    h->prev_valid = 0;
    h->hid = hid;

    if (ep->bEndpointAddress & 0x80) {
        // In endpoint
        if (ep->bmAttributes == 0x03) {
            // Allocate channel
            ESP_LOGI(HIDTAG,"Endpoint 0x%08x max size %d", ep->bEndpointAddress, ep->wMaxPacketSize);
            h->epchan = usb_ll__alloc_channel(dev->address,
                                              0x03, // Interrupt endpoint
                                              ep->wMaxPacketSize,
                                              ep->bEndpointAddress
                                             );
            usb_ll__channel_set_interval(h->epchan, ep->bInterval);


            // Start IN requests
            ESP_LOGI(HIDTAG, "hid: Submitting IN requests");

            usb_hid__submit_in(h);

            i->drvdata = h;

            r=0;
        }
    }

    return 0;
}

#define USB_HID_REQ_SET_IDLE (0x0A)

static int usb_hid__set_idle(struct usb_device *dev, usb_interface_descriptor_t *intf)
{
    struct usb_request req = {0};

    req.device = dev;
    req.target = NULL;
    req.length = 0;
    req.rptr = NULL;
    req.size_transferred = 0;
    req.direction = REQ_HOST_TO_DEVICE;
    req.control = 1;

    req.setup.bmRequestType = USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_CLASS | USB_HOST_TO_DEVICE;
    req.setup.bRequest = USB_HID_REQ_SET_IDLE;
    req.setup.wValue = __le16(0);
    req.setup.wIndex = __le16(intf->bInterfaceNumber);
    req.setup.wLength = __le16(0);

    req.channel =  dev->ep0_chan;
    ESP_LOGI(HIDTAG,"Submitting IDLE request");
    usbh__submit_request(&req);

    // Wait.
    if (usbh__wait_completion(&req)<0) {
        ESP_LOGE(HIDTAG,"Device not accepting idle!");
        return -1;
    }
    ESP_LOGI(HIDTAG, "Submitted idle request");



    return 0;

}


static void usb_hid__disconnect(struct usb_interface *intf)
{
    struct usb_hid *h = intf->drvdata;
    // Cancel any pending requests
    usb_ll__release_channel(h->epchan);

    if (h->hid) {
        hid_free(h->hid);
    }

    free(h);
    intf->drvdata = NULL;

}


const struct usb_driver usb_hid_driver = {
    .probe = &usb_hid__probe,
    .disconnect = &usb_hid__disconnect
};
