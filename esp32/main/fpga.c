#include "fpga.h"
#include "spi.h"
#include "defs.h"
#include "dump.h"
#include "esp_log.h"

static spi_device_handle_t spi0_fpga;


static void fpga__init_spi()
{
    spi__init_device(&spi0_fpga, 20000000, PIN_NUM_CS);
}

int fpga__init()
{
    uint8_t idbuf[5];

    fpga__init_spi();

    idbuf[0] = 0x9F;
    idbuf[1] = 0xAA;
    idbuf[2] = 0x55;
    idbuf[3] = 0xAA;
    idbuf[4] = 0x55;

    int r = spi__transceive(spi0_fpga, idbuf, 5);

    if (r<0) {
        printf("SPI transceive: error %d\r\n", r);
    } else {
        printf("FPGA id: ");
        dump__buffer(&idbuf[1], 4);
        printf("\r\n");
        if (idbuf[1]) {
        }
    }
    return r;
}


static uint8_t latched_flags = 0;

void fpga__set_clear_flags(uint8_t enable, uint8_t disable)
{
    uint8_t newflags = (latched_flags & ~disable) | enable;
    uint8_t buf[2];
    buf[0] = 0xEC;
    buf[1] = newflags;
    spi__transceive(spi0_fpga, buf, sizeof(buf));
    latched_flags = newflags;
}

void fpga__set_trigger(uint8_t trig)
{
    uint8_t buf[4];
    buf[0] = 0xEC;
    buf[1] = latched_flags;
    buf[2] = trig;
    buf[3] = 0x00; // Extra SPI clocks
    spi__transceive(spi0_fpga, buf, sizeof(buf));
}

void fpga__get_framebuffer(uint8_t *target)
{
    target[0] = 0xDF;
    target[1] = 0x00;
    target[2] = 0x00;
    target[3] = 0x00;
    int nlen = 4 + SPECTRUM_FRAME_SIZE;
//    int len = -1;

    spi__transceive(spi0_fpga, target, nlen);
    // Notify frame grabbed.
    target[0] = 0xEF;
    target[1] = 0x00;
    spi__transceive(spi0_fpga, target, 2);

}

void fpga__set_register(uint8_t reg, uint32_t value)
{
    uint8_t buf[6];
    buf[0] = 0xED;
    buf[1] = reg;
    buf[2] = (value >> 24)& 0xff;
    buf[3] = (value >> 16)& 0xff;
    buf[4] = (value >> 8)& 0xff;
    buf[5] = (value)& 0xff;

    spi__transceive(spi0_fpga, buf, sizeof(buf));
}

uint32_t fpga__get_register(uint8_t reg)
{
    uint8_t buf[6];
    buf[0] = 0xEE;
    buf[1] = reg;
    buf[2] = 0xAA;
    buf[3] = 0xAA;
    buf[4] = 0xAA;
    buf[5] = 0xAA;

    spi__transceive(spi0_fpga, buf, sizeof(buf));

    return getbe32(&buf[2]);
}

void fpga__set_capture_mask(uint32_t mask)
{
    fpga__set_register(REG_CAPTURE_MASK, mask);
}

void fpga__set_capture_value(uint32_t value)
{
    fpga__set_register(REG_CAPTURE_VAL, value);
}

uint32_t fpga__get_capture_status()
{
    uint8_t buf[6];
    buf[0] = 0xE2;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;

    spi__transceive(spi0_fpga, buf, sizeof(buf));
    uint32_t ret = getbe32( &buf[2] );
    return ret;
}

int fpga__get_captures(uint8_t *target)
{
    target[0] = 0xE0;
    target[1] = 0x00;
    int nlen = 2+ (5 * 2048);
    //int len = -1;
    spi__transceive(spi0_fpga, target, nlen);
    return nlen;
}

int fpga__upload_rom_chunk(uint16_t offset, uint8_t *buffer_sub3, unsigned len)
{
    // Upload chunk
    buffer_sub3[0] = 0xE1;
    buffer_sub3[1] = (offset>>8) & 0xFF;
    buffer_sub3[2] = offset & 0xFF;

    {
        dump__buffer(buffer_sub3, 8);
    }

    spi__transceive(spi0_fpga, buffer_sub3, 3 + len);

    return len;
}