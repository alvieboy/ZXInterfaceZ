#include <inttypes.h>
#include <stdio.h>

static void dump(const char *t, const uint8_t *buffer, size_t len)
{
    printf("%s: [",t);
    while (len--) {
        printf(" %02x", *buffer++);
    }
    printf(" ]\n");
}


void fpga_do_transaction(uint8_t *buffer, size_t len)
{
    //dump("SPI IN: ",buffer,len);
    switch (buffer[0]) {
    case 0x9F:
        buffer[1] = 0xA5;
        buffer[2] = 0xA5;
        buffer[3] = 0x10;
        buffer[4] = 0x03;
        break;
    }
}
