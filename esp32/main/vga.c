#include "vga.h"
#include "fpga.h"
#include "nvs.h"

static void vga__applymode(uint8_t mode)
{
    switch (mode) {
    case 0:
        fpga__clear_flags(FPGA_FLAG_VIDMODE0 | FPGA_FLAG_VIDMODE1);
        break;
    case 1:
        fpga__set_clear_flags(FPGA_FLAG_VIDMODE0, FPGA_FLAG_VIDMODE1);
        break;
    case 2:
        fpga__set_clear_flags(FPGA_FLAG_VIDMODE1, FPGA_FLAG_VIDMODE0);
        break;
    case 3:
        fpga__set_flags(FPGA_FLAG_VIDMODE0 | FPGA_FLAG_VIDMODE1);
        break;
    }
}

void vga__init(void)
{
    vga__applymode( vga__getmode() );
}

int vga__setmode(uint8_t mode)
{
    if (mode>3)
        mode = 3;

    nvs__set_u8("vgamode", mode);
    vga__applymode(mode);

    return 0;
}

int vga__getmode(void)
{
    return nvs__u8("vgamode",0);
}
