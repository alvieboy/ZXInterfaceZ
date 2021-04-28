#include "rom_hook.h"
#include "fpga.h"
#include "log.h"

#define TAG "ROM_HOOK"

typedef uint16_t hook_mask_t;

static hook_mask_t hook_usage_bitmap = 0;



static int8_t rom_hook_defaults[5] = { -1 };

static hook_mask_t rom_hook__get_free_hook_mask()
{
    return ~hook_usage_bitmap & (hook_usage_bitmap+1);
}

int rom_hook__add(uint16_t start, masklen_t masklen, uint8_t flags)
{
    hook_mask_t newmask = rom_hook__get_free_hook_mask();

    if (newmask==0) {
        ESP_LOGE(TAG, "MAX hooks exceeded!");
        return -1;
    }

    uint8_t index = __builtin_ctz(newmask);

    ESP_LOGI(TAG,"Adding hook 0x%04x len %d flags=0x%02x (index %d)",start,masklen,flags,index);
    int r = fpga__write_hook(index, start, masklen, flags);

    if (r<0)
        return -r;

    hook_usage_bitmap |= newmask;

    return index;
}


void rom_hook__remove(int hook)
{
    if (hook<0)
        return;
    hook_mask_t mask = (1<<hook);

    ESP_LOGI(TAG,"Removing hook %d mask 0x%02x", hook, mask);
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

    rom_hook_defaults[0] = rom_hook__add_pre_set(rom, 0x0767, MASK_LEN_1); // LOAD
    rom_hook_defaults[1] = rom_hook__add_pre_set(rom, 0x04D7, MASK_LEN_1); // SAVE
    rom_hook_defaults[2] = rom_hook__add_post_reset(rom, 0x01FFE, MASK_LEN_1); // Will have a RET instruction
    rom_hook_defaults[3] = rom_hook__add_post_set(rom, 0x00A0, MASK_LEN_1); // Will have a RET instruction
    rom_hook_defaults[4] = rom_hook__add_pre_set(rom, 0x0008, MASK_LEN_1); // RST 8

    return 0;
}

void rom_hook__disable_defaults()
{
    int i;
    for (i=0; i<sizeof(rom_hook_defaults)/sizeof(rom_hook_defaults[0]);i++) {
        if (rom_hook_defaults[i]>=0) {
            rom_hook__remove(rom_hook_defaults[i]);
            rom_hook_defaults[i] = -1;
        }
    }
}

void rom_hook__dump()
{
#if 0
    ESP_LOGI(TAG, "Current bitmap: %02x", hook_usage_bitmap);
    ESP_LOGI(TAG, "Default hooks: %d %d %d",
             rom_hook_defaults[0],
             rom_hook_defaults[1],
             rom_hook_defaults[2]);

    union u32 dest[8];

    fpga__read_hooks(&dest[0].w32);
    BUFFER_LOGI(TAG,"Hook conf:", &dest[0].w8[0], 4*8);
#endif
}
