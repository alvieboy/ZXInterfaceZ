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

struct opstatus_resource opstatusresource = {
    .r = OPSTATUS_RESOURCE_DEF,
    .status = 0xFF,
    .str = ""
};

struct directory_resource directoryresource = {
    .r = DIRECTORY_RESOURCE_DEF,
    .alloc_size = 0,
    .buffer = NULL,
    .entries = 0,
    .filter = 0
};

struct int8_resource videomodeconfigresource = {
    .r = INT8_RESOURCE_DEF,
    .valptr = NULL,
    .latched_val = 0
};
