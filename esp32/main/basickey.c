#include "basickey.h"
#include <string.h>
#include "rom_hook.h"

static uint8_t keylen = 0;
static uint8_t keypos = 0;
static uint8_t keybuf[16];
static int8_t basic_inject_hook = -1;

bool basickey__has_inject(void)
{
    return keypos!=keylen;
}

uint8_t basickey__get_inject()
{
    uint8_t v = keybuf[keypos++];
    if (keypos>=keylen) {
        // Remove hook
        if (basic_inject_hook>=0) {
            rom_hook__remove(basic_inject_hook);
            basic_inject_hook = -1;
        }
    }

    return v;
}

void basickey__clearinject(void)
{
    keypos=0;
    keylen=0;
}

void basickey__inject(const uint8_t *keys, unsigned len)
{
    // This currently removes everything
    basickey__clearinject();
    memcpy(keybuf, keys, len);
    keylen = len;

    if (basic_inject_hook<0) {
        basic_inject_hook = rom_hook__add_pre_set(model__get_basic_rom(), 0x0F38, 1); // ED_LOOP
    }

}
