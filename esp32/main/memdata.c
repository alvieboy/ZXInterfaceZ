#include "memdata.h"

#include <string.h>

static uint8_t memdata[256];
static uint8_t memdatasize=0;

const uint8_t *memdata__retrieve(uint8_t *size)
{
    *size = memdatasize;
    return memdata;
}

void memdata__store(const uint8_t *data, uint8_t size)
{
    memcpy(memdata, data, size);
    memdatasize = size;
}


/* Returns actual length */
uint8_t memdata__analyse_request(uint16_t address, uint8_t len,
                                 struct memdata_request *out, uint8_t *num_requests)
{
    int request = 0;
    uint8_t newlen;
    uint8_t actual_len = len;
    /*
     Split single request into eventual multiple memory areas.

     0x0000 -> 0x3FFF  ROM
     0x4000 -> 0x5FFF  SAVED_RAM
     0x6000 -> 0xFFFF  RAM

     */

    /* Avoid crossing memory boundary */

    uint16_t end_address = (address + len)-1;
    if (end_address < address) {
        // Overflow,
        actual_len -= (uint8_t)(end_address+1);
        len = actual_len;
    }
    end_address = (address + actual_len)-1; // Recalculate

    // Check if we crossed any border

    if ((address <= 0x3FFF) && (end_address>=0x4000)) {
        // ROM/SAVED_RAM
        newlen = (0x4000-address);
        out[request].address = address;
        out[request].type = ROM;
        out[request].len = newlen;
        address = 0x4000;
        len -= newlen;
        request++;
    } else if ((address <= 0x5FFF) && (end_address>=0x6000)) {
        // SAVED_RAM/RAM
        newlen = (0x6000-address);
        out[request].address = address;
        out[request].type = SAVED_RAM;
        out[request].len = newlen;
        address = 0x6000;
        len -= newlen;
        request++;
    }

    if (len) {
        out[request].address = address;
        out[request].len = len;
        if (address<0x4000) {
            out[request].type = ROM;
        } else if (address<0x6000) {
            out[request].type = SAVED_RAM;
        } else {
            out[request].type = RAM;
        }
        request++;
    }
    *num_requests = request;
    return actual_len;
}
