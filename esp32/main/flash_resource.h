#include "resource.h"

struct flashresourcedata {
    uint8_t type;
    uint16_t len;
    uint8_t data[0];
} __attribute__((packed));

struct flash_resource {
    struct resource r;
    const struct flashresourcedata *data;
};


struct flash_resource *flash_resource__find(uint8_t id);

void flash_resource__init(void);
