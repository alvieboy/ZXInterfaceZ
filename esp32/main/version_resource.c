#include "resource.h"
#include "fpga.h"
#include "version.h"
#include "version_resource.h"

static uint8_t versionstring[128];
static uint8_t versionstringlen;

void version_resource__update(struct resource *res)
{
    char *p = (char*)&versionstring[1];

    // Reset transmit fifo
    unsigned fpga_id = fpga__read_id();

    int len = sprintf(p,"%s FPGA %02x.%02x r%d",
                      version,
                      (fpga_id>>16) & 0xff,
                      (fpga_id>>8) & 0xff,
                      (fpga_id) & 0xff);

    // Strlen
    versionstring[0] = len;
    versionstringlen = len + 1;
}

int version_resource__sendToFifo(struct resource *res)
{
    return fpga__load_resource_fifo(versionstring, versionstringlen, RESOURCE_DEFAULT_TIMEOUT);
}

uint8_t version_resource__type(struct resource *res)
{
    return RESOURCE_TYPE_STRING;
}

uint16_t version_resource__len(struct resource *res)
{
    return versionstringlen;
}

