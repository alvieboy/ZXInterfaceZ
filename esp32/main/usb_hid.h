#include "usbh.h"

typedef struct
{
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdHID;
  uint8_t bCountryCode;
  uint8_t bNumDescriptors;
  uint8_t bReportDescriptorType;
  uint16_t wItemLength;
} __attribute__((packed)) usb_hid_descriptor_t;


extern const struct usb_driver usb_hid_driver;
