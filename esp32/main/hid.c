#include "hid.h"
#include "usb_hid.h"
#include <stdlib.h>

uint32_t hid__get_id(const hid_device_t *hiddev)
{
    if (hiddev->bus==HID_BUS_USB) {
        return usb_hid__get_id((struct usb_hid*)hiddev);
    }
    return 0;
}

const char *hid__get_serial(const hid_device_t *hiddev)
{
    if (hiddev->bus==HID_BUS_USB) {
        return usb_hid__get_serial((struct usb_hid*)hiddev);
    }
    return NULL;
}

int hid__get_interface(const hid_device_t *hiddev)
{
    if (hiddev->bus==HID_BUS_USB) {
        return usb_hid__get_interface((struct usb_hid*)hiddev);
    }
    return 0;
}


void hid__get_devices(hid_device_t *devices, unsigned *num, unsigned max)
{
    (*num) = 0;

}
const char* hid__get_driver_name(const hid_device_t*hiddev)
{
    if (hiddev->bus==HID_BUS_USB) {
        return "usb_hid";
    }
    return NULL;
}

bool hid__has_multiple_reports(struct hid *h)
{
    if (h->reports && h->reports->next!=NULL)
        return true;
    return false;

}
int hid__number_of_reports(struct hid *h)
{
    hid_report_t * report = h->reports;
    int count = 0;
    do {
        if (report) {
            count ++;
            report = report->next;
        }
    } while (report);
    return count;
}

hid_report_t *hid__find_report_by_id(struct hid *h, uint8_t report_id, uint8_t *report_index_out)
{
    int idx = 0;

    hid_report_t *report = h->reports;
    while (report) {
        if (report->id==report_id) {
            *report_index_out = idx;
            return report;
        }
        idx++;
        report = report->next;
    }
    return NULL;
}
