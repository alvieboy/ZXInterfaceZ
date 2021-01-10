#include "rom_hook.h"
#include "fpga.h"

static uint8_t hook_usage_bitmap = 0;
static int8_t rom_hook_defaults[3] = { -1 };

static uint8_t rom_hook__get_free_hook_mask()
{
    return ~hook_usage_bitmap & (hook_usage_bitmap+1);
}

int rom_hook__add(uint16_t start, uint8_t len, uint8_t flags)
{

    uint8_t newmask = rom_hook__get_free_hook_mask();
    if (newmask==0)
        return -1;

    uint8_t index = __builtin_ctz(newmask);

    len--; // 0 will become 255.

    int r = fpga__write_hook(index, start, len, flags);

    if (r<0)
        return -r;

    hook_usage_bitmap |= newmask;
    return 0;
}


void rom_hook__remove(int hook)
{
    uint8_t mask = (1<<hook);
    fpga__disable_hook(hook);
    hook_usage_bitmap &= ~mask;
}

int rom_hook__enable_defaults()
{
    uint8_t rom;

    model_t model = model__get();

    switch (model) {
    case MODEL_16K: /* Fall-through */
    case MODEL_48K:
        rom = 0;
        break;
    case MODEL_128K:
        rom = 1;
        break;
    default:
        return -1;
        break;
    }

    rom_hook_defaults[0] = rom_hook__add_pre_set(rom, 0x0767, 1); // LOAD
    rom_hook_defaults[1] = rom_hook__add_pre_set(rom, 0x0970, 1); // SAVE
    rom_hook_defaults[2] = rom_hook__add_post_reset(rom, 0x01FFE, 1); // Will have a RET instruction

    return 0;
}

void rom_hook__disable_defaults()
{
    if (rom_hook_defaults[0]>=0) {
        rom_hook__remove(rom_hook_defaults[0]);
        rom_hook_defaults[0] = -1;
    }
    if (rom_hook_defaults[1]>=0) {
        rom_hook__remove(rom_hook_defaults[1]);
        rom_hook_defaults[1] = -1;
    }
    if (rom_hook_defaults[2]>=0) {
        rom_hook__remove(rom_hook_defaults[1]);
        rom_hook_defaults[2] = -1;
    }
}
