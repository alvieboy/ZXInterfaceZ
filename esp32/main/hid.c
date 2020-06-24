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
