#include <inttypes.h>
#include "fpga.h"

static uint64_t keys = 0;

void keyboard__init(void)
{
    keys = 0;

    fpga__set_register(REG_KEYB1_DATA, 0);
    fpga__set_register(REG_KEYB2_DATA, 0);

    fpga__set_config1_bits(CONFIG1_KBD_ENABLE);

    //fpga__set_flags(FPGA_FLAG_ULAHACK);
}

static void keyboard__update(void)
{
    fpga__set_register(REG_KEYB1_DATA, keys & 0xFFFFFFFF);
    fpga__set_register(REG_KEYB2_DATA, (keys>>32) & 0xFFFFFFFF);
}

void keyboard__press(uint8_t key)
{
    if (key>39)
        key = 39;

    keys |= (1ULL << key);
    keyboard__update();
}

void keyboard__release(uint8_t key)
{
    if (key>39)
        key = 39;

    keys &= ~(1ULL << key);
    keyboard__update();
}

