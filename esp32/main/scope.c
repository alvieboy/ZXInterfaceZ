#include "scope.h"
#include <inttypes.h>
#include "fpga.h"
#include "byteops.h"

static const char *scope_names_nontrig[] = {
    "D0",
    "D1",
    "D2",
    "D3",
    "D4",
    "D5",
    "D6",
    "D7",
    "FORCEROMCS0",
    "FORCEROMCS2A",
    "/REQACK",
    "/CMDFIFOEMPTY",
    "DBUSDIR",
    "ROMPAGESEL0"
};

static const char *scope_names_trig[] = {
    "A0",
    "A1",
    "A2",
    "A3",
    "A4",
    "A5",
    "A6",
    "A7",
    "A8",
    "A9",
    "A10",
    "A11",
    "A12",
    "A13",
    "A14",
    "A15",
    "/CK",
    "/INT",
    "/MREQ",
    "/IORQ",
    "/RD",
    "/WR",
    "/M1",
    "/RFSH",
    "/WAIT",
    "/NMI",
    "/RESET",
    "FORCEROMCS",
    "FORCEIORQULA",
    "USB_INT",
    "/INT",
    "SPECT_INT"
};

int scope__get_group_size(scope_group_t group)
{
    int r = -1;
    switch (group) {
    case SCOPE_GROUP_NONTRIG:
        r = sizeof(scope_names_nontrig)/sizeof(scope_names_nontrig[0]);
        break;
    case SCOPE_GROUP_TRIG:
        r = sizeof(scope_names_trig)/sizeof(scope_names_trig[0]);
        break;
    default:
        break;
    }
    return r;
}

const char *scope__get_signal_name(scope_group_t group, int index)
{
    const char *name = NULL;
    switch (group) {
    case SCOPE_GROUP_NONTRIG:
        name = scope_names_nontrig[index];
        break;
    case SCOPE_GROUP_TRIG:
        name = scope_names_trig[index];
        break;
    default:
        break;
    }
    return name;
}


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
    // 64 samples are 256 bytes. Offset is 4 bits.
    address |= ((unsigned)offset&15)<<8;
    fpga__read_capture_block(address, dest, 256);
}
