#include "dump.h"
#include <stdio.h>
#include "esp_log.h"
#include "defs.h"

/**
 \ingroup misc
 \brief Dump a buffer to serial port
 \param data Buffer to dump
 \param len Size of buffer, in bytes
 */
void dump__buffer(const uint8_t *data, unsigned len)
{
    printf("[");
    while (len--) {
        printf(" %02x", *data++);
    }
    printf(" ]\r\n");
}
