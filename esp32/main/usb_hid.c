#include "usb_driver.h"
#include "usb_defs.h"
#include "usbh.h"
#include <stdlib.h>
#include "usb_descriptor.h"
#include "usb_hid.h"
#include <stdbool.h>
#include "dump.h"
#include "hid.h"
#include <string.h>
#include "log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define HIDTAG "HID"
#define HIDDEBUG(x...) LOG_DEBUG(DEBUG_ZONE_HID, HIDTAG, x)
#define HIDLOG(x...) ESP_LOGI(HIDTAG, x)

static int usb_hid__set_idle(struct usb_device *dev, usb_interface_descriptor_t *intf, uint8_t report);

struct usb_hid_report_payload
{
    uint8_t len;
    uint8_t *data;
};

struct usb_hid
{
    hid_device_t hiddev;
    uint8_t epchan;
    uint8_t max_report_size;
   // uint8_t seq:1;
    struct usb_device *dev;
    struct usb_interface *intf;
    struct hid *hid;
    uint8_t *report_desc;
    uint8_t num_reports;
    struct usb_hid_report_payload *payloads;
};

static void usb_hid__allocate_payloads(struct usb_hid *hid)
{
    struct hid *h =  hid->hid;
    unsigned max_report_size = 0;
    int count = hid__number_of_reports(h);
    if (count<1)  {
        ESP_LOGE(HIDTAG, "No reports found!");
        hid->payloads = NULL;
        return;
    }
    hid->payloads = calloc( count, sizeof(struct usb_hid_report_payload) );
    if (!hid->payloads) {
        ESP_LOGE(HIDTAG, "Cannot allocate memory");
    }
    hid_report_t *report = h->reports;
    for (int i=0;i<count;i++) {
        unsigned this_report_size = (report->size + 7) >> 3; // Make sure we have at least 1 byte.
        hid->payloads[i].data = malloc( this_report_size );
        hid->payloads[i].len = 0;
        if (this_report_size>max_report_size)
            max_report_size = this_report_size;
    }
    hid->num_reports = count;
    hid->max_report_size = max_report_size;
}

static void usb_hid__free_payloads(struct usb_hid *hid)
{
    if (hid->payloads) {
        for (int i=0;i<hid->num_reports;i++) {
            free( hid->payloads[i].data );
        }
    }
    free(hid->payloads);
}


uint32_t usb_hid__get_id(const struct usb_hid *usbhid)
{
    return usbh__get_device_id(usbhid->dev);
}

const char *usb_hid__get_serial(const struct usb_hid *usbhid)
{
    return usbhid->dev->serial;
}

int usb_hid__get_interface(const struct usb_hid *usbhid)
{
    // TODO: fix this to report actual interface based on alternate setting

    return usbhid->intf->descriptors[0]->bInterfaceNumber;
}

static int usb_hid__submit_in(struct usb_hid *h);

static void usb_hid__parse_report(struct usb_hid *hid, const uint8_t *payload, int payload_size)
{
    uint8_t new_value;
    uint8_t old_value;
    bool propagate = false;

    if (!hid->hid)
        return;

    struct hid *h = hid->hid;

    unsigned entry_index = 0;

    uint8_t report_id = 0;
    uint8_t report_index = 0;

    if (hid__has_multiple_reports(h)) {
        if (payload_size<2) {
            ESP_LOGE(HIDTAG,"Multiple reports for device, but only %d bytes", payload_size);
            return;
        }
        // Extract report index.
        report_id = *payload++;
        payload_size--;
    }

    hid_report_t *report = hid__find_report_by_id(h, report_id, &report_index);
    if (!report || (report_index > hid->num_reports) ) {
        ESP_LOGE(HIDTAG,"Cannot find report with id %d", report_id);
        return;
    }
    // Get payload pointer.

    struct usb_hid_report_payload *stored_payload = &hid->payloads[report_index];


    // Fast compare.
    if (stored_payload->len == payload_size ) {
        if (memcmp(payload, stored_payload->data, payload_size)==0)
            return;
    }

    hid_field_t *field = report->fields;

    HIDDEBUG("Field ");
    while (field) {
        int i;
        HIDDEBUG(" > start %d len %d", field->report_offset, field->report_size);
        for (i=0;i<field->report_count;i++) {

            propagate = false;

            HIDDEBUG(" >> index %d", i);

            if (hid__extract_field_aligned(field, i, payload, &new_value)<0)
                goto out;

            if ( stored_payload->len > 0 ) {
                if (hid__extract_field_aligned(field, i, stored_payload->data, &old_value)<0)
                    goto out;
                if (new_value!=old_value)
                    propagate = true;
            } else {
                propagate = true;
            }
            if (propagate) {

                HIDDEBUG("Field changed start %d len %d (entry index %d) prev %d now %d",
                         field->report_offset + (i*field->report_size),
                         field->report_size,
                         entry_index,
                         old_value,
                         new_value
                        );

                hid__field_entry_changed_callback((hid_device_t*)hid, field, entry_index, new_value);
            }
            entry_index++;
        }
        field = field->next;
    }
out:
    memcpy(stored_payload->data, payload, payload_size);
    stored_payload->len = payload_size;

}

static int usb_hid__transfer_completed(uint8_t channel, uint8_t status, void*userdata)
{
    struct usb_hid *h = (struct usb_hid*)userdata;
    union {
        uint8_t w8[64];
        uint32_t w32[16];
    } payload;
    uint8_t rxlen = sizeof(payload);

    if (!h) {
        ESP_LOGE(HIDTAG, "Invalid private pointer!!!");
        return -1;
    }
    if (status&0x01) {

        HIDDEBUG("got data stat %d", status);
        // Fetch data from ....
        usb_ll__read_in_block(h->epchan, &payload.w32[0], &rxlen);
        HIDDEBUG("Parsing report");
        usb_hid__parse_report(h, &payload.w8[0], rxlen);

        //HIDDEBUG("Copying data");
        //h->seq = !h->seq;
        //memcpy(h->prev_payload, h->payload, rxlen);
        //h->prev_size = rxlen;
    } else {
        ESP_LOGE(HIDTAG,"error %d, resubmitting", status);
    }

    usb_hid__submit_in(h);

    return 0;
}

static int usb_hid__submit_in(struct usb_hid *m)
{
    unsigned size = m->max_report_size;
    if (m->num_reports>1)
        size++; // Include report ID

    return usb_ll__submit_request(m->epchan,
                                  PID_IN,
                                  NULL,
                                  size, // Report descriptor size!!!
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
    uint16_t size_transferred;

    HIDLOG("     ***** PROBING INTERFACE %d altSetting %d **** class=%02x %p %p\n",
           intf->bInterfaceNumber,
           intf->bAlternateSetting,
           intf->bInterfaceClass,
           intf,
           i);

    if (intf->bInterfaceClass == 0x03) {
        /*
        if (intf->bInterfaceSubClass == 0x00) { // Not boot
            if (intf->bInterfaceProtocol == 0x00) { // Not mouse/keyboard
                valid = true;
            }
            }*/
        valid = true;
    }

    if (!valid)
        return false;

    usb_hid_descriptor_t *hidd = (usb_hid_descriptor_t*)usb__find_descriptor( intf, intf_len, USB_DESC_TYPE_HID, 0);

    if (!hidd) {
        ESP_LOGE(HIDTAG, "Cannot find HID descriptor");
        return -1;
    }

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
                                 report_desc_len,
                                 &size_transferred);
    if (r<0) {
        ESP_LOGE(HIDTAG, "Cannot read HID report descriptor");
        free( report_desc );
        return -1;
    }

    HIDDEBUG("HID report descriptor len %d", report_desc_len);
    if (DEBUG_ENABLED(DEBUG_ZONE_USBLL)) {
        dump__buffer(report_desc, report_desc_len);
    }

    struct hid *hid = hid__parse(report_desc, report_desc_len);

    if (NULL==hid) {
        free( report_desc );
        return -1;
    }

#if 0
    // Send SET_IDLE request for all relevant reports
    const hid_report_t *report = hid->reports;
    while (report) {
        if (usb_hid__set_idle(dev, intf, report->id)<0) {
            free( report_desc );
            return -1;
        }
        report = report->next;
    };
#else
    if (usb_hid__set_idle(dev, intf, 0)<0) {
        free( report_desc );
        return -1;
    }

#endif

    struct usb_hid *h =  malloc(sizeof(struct usb_hid)); // TBD.

    h->hiddev.bus = HID_BUS_USB;
    h->dev = dev;
    h->intf = i;
    //h->seq = 0;
    h->hid = hid;

    usb_hid__allocate_payloads(h);

    if (ep->bEndpointAddress & 0x80) {
        // In endpoint
        if (ep->bmAttributes == 0x03) {
            // Allocate channel
            ESP_LOGI(HIDTAG,"Endpoint 0x%08x max size %d", ep->bEndpointAddress, ep->wMaxPacketSize);
            h->epchan = usb_ll__alloc_channel(dev->address,
                                              0x03, // Interrupt endpoint
                                              ep->wMaxPacketSize,
                                              ep->bEndpointAddress,
                                              dev->fullspeed,
                                              dev
                                             );
            usb_ll__channel_set_interval(h->epchan, ep->bInterval);


            // Start IN requests
            HIDDEBUG("Submitting IN requests");

            usb_hid__submit_in(h);

            i->drvdata = h;

            r=0;
        }
    }

    return 0;
}

#define USB_HID_REQ_SET_IDLE (0x0A)
#define HID_IDLE_REQUEST_TIMEOUT (500 / portTICK_RATE_MS)

static int usb_hid__set_idle(struct usb_device *dev, usb_interface_descriptor_t *intf, uint8_t report)
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
    req.setup.wValue = __le16(0 | report);
    req.setup.wIndex = __le16(intf->bInterfaceNumber);
    req.setup.wLength = __le16(0);

    req.channel =  dev->ep0_chan;
    HIDDEBUG("Submitting IDLE request interface %d", intf->bInterfaceNumber );
    usbh__submit_request(&req);

    // Wait.
    if (usbh__wait_completion(&req, HID_IDLE_REQUEST_TIMEOUT)<0) {
        ESP_LOGE(HIDTAG,"Device not accepting idle!");
        return -1;
    }

    return 0;

}


static void usb_hid__disconnect(struct usb_device *dev, struct usb_interface *intf)
{
    struct usb_hid *h = intf->drvdata;
    // Cancel any pending requests
    usb_ll__release_channel(h->epchan);

    if (h->hid) {
        hid__free(h->hid);
    }

    usb_hid__free_payloads(h);
    free(h);
    intf->drvdata = NULL;

}


const struct usb_driver usb_hid_driver = {
    .probe = &usb_hid__probe,
    .disconnect = &usb_hid__disconnect
};


