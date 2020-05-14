#include "dump.h"
#include <stdio.h>
#include "esp_log.h"
#include "defs.h"

void dump__buffer(const uint8_t *data, unsigned len)
{
    printf("[");
    while (len--) {
        printf(" %02x", *data++);
    }
    printf(" ]\r\n");
}
