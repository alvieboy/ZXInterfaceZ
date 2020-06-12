#include <inttypes.h>
#include "esp_log.h"

#define DEBUG_ZONE_HID (1<<0)
#define DEBUG_ZONE_USBLL (1<<1)
#define DEBUG_ZONE_USBH (1<<2)

extern uint32_t loglevel;

#define DEBUG_ENABLED(zone) (loglevel&(zone))

#define LOG_DEBUG(zone,tag,x...) do { \
    if (loglevel&zone) { \
    ESP_LOGI(tag,x);\
    } \
    } while (0)
