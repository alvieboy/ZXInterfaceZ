#include "scope.h"
#include <inttypes.h>
#include "fpga.h"
#include "byteops.h"

void scope__start(uint8_t clockdiv)
{
    uint8_t buf[4];

    putle32(buf, 0x80000000 | (clockdiv & 7));

    fpga__write_capture_block(0x0010, buf, 4);
}

void scope__set_triggers(uint32_t mask,
                         uint32_t value,
                         uint32_t edge)
{
    uint8_t buf[12];
    uint8_t *ptr = buf;
    ptr = putle32(ptr, mask);
    ptr = putle32(ptr, value);
    ptr = putle32(ptr, edge);

    fpga__write_capture_block(0x0000, buf, 12);
}

void scope__get_capture_info(uint32_t *status, uint32_t *counter, uint32_t *trig_address)
{
    uint8_t buf[12];
    fpga__read_capture_block(0x0000, buf, 12);
    *status = extractle32(&buf[0]);
    *counter = extractle32(&buf[4]);
    *trig_address = extractle32(&buf[8]);
}

void scope__get_capture_data_block64(uint8_t channel, uint8_t offset, uint8_t*dest)
{
    unsigned address = (1<<13);
    if (channel)
        address  |= (1<<12);
    // 11 to 0 is RAM individual address.
    // 64 samples are 256 bytes. Offset is 3 bits.
    address |= ((unsigned)offset&7)<<8;
    fpga__read_capture_block(address, dest, 256);
}
