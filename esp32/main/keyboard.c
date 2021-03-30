/**
 * \defgroup keyboard
 * \brief ZX Spectrum keyboard interfacing
 */
#include <inttypes.h>
#include "fpga.h"
#include "keyboard.h"
#include "string.h"

static uint64_t keys = 0;

/**
 * \ingroup keyboard
 * \brief Initialise the keyboard subsystem
 */
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

/**
 * \ingroup keyboard
 * \brief Press a key on the keyboard. 
 */
void keyboard__press(uint8_t key)
{
    if (key>39)
        key = 39;

    keys |= (1ULL << key);
    keyboard__update();
}

/**
 * \ingroup keyboard
 * \brief Release a key on the keyboard.
 */
void keyboard__release(uint8_t key)
{
    if (key>39)
        key = 39;

    keys &= ~(1ULL << key);
    keyboard__update();
}

static struct  {
    const char *name;
    uint8_t value;
} keymap[] = {
    { "shift", SPECT_KEYIDX_SHIFT },
    { "z", SPECT_KEYIDX_Z },
    { "x", SPECT_KEYIDX_X },
    { "c", SPECT_KEYIDX_C },
    { "v", SPECT_KEYIDX_V },
    { "a", SPECT_KEYIDX_A },
    { "s", SPECT_KEYIDX_S },
    { "d", SPECT_KEYIDX_D },
    { "f", SPECT_KEYIDX_F },
    { "g", SPECT_KEYIDX_G },
    { "q", SPECT_KEYIDX_Q },
    { "w", SPECT_KEYIDX_W },
    { "e", SPECT_KEYIDX_E },
    { "r", SPECT_KEYIDX_R },
    { "t", SPECT_KEYIDX_T },
    { "1", SPECT_KEYIDX_1 },
    { "2", SPECT_KEYIDX_2 },
    { "3", SPECT_KEYIDX_3 },
    { "4", SPECT_KEYIDX_4 },
    { "5", SPECT_KEYIDX_5 },
    { "0", SPECT_KEYIDX_0 },
    { "9", SPECT_KEYIDX_9 },
    { "8", SPECT_KEYIDX_8 },
    { "7", SPECT_KEYIDX_7 },
    { "6", SPECT_KEYIDX_6 },
    { "p", SPECT_KEYIDX_P },
    { "o", SPECT_KEYIDX_O },
    { "i", SPECT_KEYIDX_I },
    { "u", SPECT_KEYIDX_U },
    { "y", SPECT_KEYIDX_Y },
    { "enter", SPECT_KEYIDX_ENTER },
    { "l", SPECT_KEYIDX_L },
    { "x", SPECT_KEYIDX_K },
    { "j", SPECT_KEYIDX_J },
    { "h", SPECT_KEYIDX_H },
    { "space", SPECT_KEYIDX_SPACE },
    { "sym", SPECT_KEYIDX_SYM },
    { "m", SPECT_KEYIDX_M },
    { "n", SPECT_KEYIDX_N },
    { "b", SPECT_KEYIDX_B }
};

/**
 * \ingroup keyboard
 * \brief Get the key identifier by name
 */
uint8_t keyboard__get_key_by_name(const char *name)
{
    unsigned i;
    for (i=0;i<sizeof(keymap)/sizeof(keymap[0]);i++) {
        if (strcmp(name, keymap[i].name)==0) {
            return keymap[i].value;
        }
    }
    return SPECT_KEYIDX_UNKNOWN;
}

/**
 * \ingroup keyboard
 * \brief Get the name for a particular key
 */
const char *keyboard__get_name_by_key(const uint8_t key)
{
    unsigned i;
    for (i=0;i<sizeof(keymap)/sizeof(keymap[0]);i++) {
        if (key == keymap[i].value ) {
            return keymap[i].name;
        }
    }
    return NULL;
}
