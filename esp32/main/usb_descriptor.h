
#include "usb_defs.h"

usb_descriptor_header_t *usb__find_descriptor(const void*data, int datalen, uint8_t type, int index);
usb_interface_descriptor_t *usb__get_interface_descriptor(usb_config_descriptor_t *conf, int index);
