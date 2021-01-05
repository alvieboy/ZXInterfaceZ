#include "rom_hook.h"
#include "fpga.h"

static uint8_t hook_usage_bitmap = 0;

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

