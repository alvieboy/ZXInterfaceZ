#include "usb_descriptor.h"
#include <stdlib.h>

usb_descriptor_header_t *usb__find_descriptor(const void*data, int datalen, uint8_t type, int index)
{
    const uint8_t *dptr = (const uint8_t *)data;

    if (datalen<sizeof(usb_descriptor_header_t)) {
        return NULL;
    }
    //DEBUG("Searching for %02x\n", type);
    do {
        usb_descriptor_header_t *hdr = (usb_descriptor_header_t *)dptr;
      //  DEBUG(" >> descr %02x len %d\n", hdr->bDescriptorType, hdr->bLength);

        if (hdr->bDescriptorType==type) {
            if (index==0) {
        //        DEBUG(" >>> return descr\n");
                return hdr;
            }
         //   DEBUG("Skipping, index %d\n", index);
            index--;
        }
        // Skip past this descriptor
        datalen -= hdr->bLength;
       // DEBUG(" >> datalen %d\n", datalen);
        if (datalen<=0) {
            return NULL;
        }
        dptr += hdr->bLength;

    } while (datalen>0);
    return NULL;
}


usb_interface_descriptor_t *usb__get_interface_descriptor(usb_config_descriptor_t *conf, int index)
{
    return (usb_interface_descriptor_t*)usb__find_descriptor(conf, __le16(conf->wTotalLength), USB_DESC_TYPE_INTERFACE, index);
}
