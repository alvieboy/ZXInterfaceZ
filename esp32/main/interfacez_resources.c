#include "interfacez_resources.h"
#include "wifi.h"

struct aplist_resource aplistresource = {
    .r = APLIST_RESOURCE_DEF,
    .buf = NULL,
    .buflen = 0
};

struct resource versionresource = {
    .type = &version_resource__type,
    .len  = &version_resource__len,
    .sendToFifo  = &version_resource__sendToFifo,
    .update = &version_resource__update
};

struct status_resource statusresource = {
    .r = STATUS_RESOURCE_DEF,
    .status = 0
};

struct string_resource wificonfigresource = {
    .r = STRING_RESOURCE_DEF,
    .str = wifi_ssid
};
