#include "spectrum_kbd.h"
#include <stdlib.h>


static char CAPSSHIFT_KEYS[39] = {
    'B', 'H', 'Y', KEY_DOWN, KEY_LEFT, 'T', 'G', 'V',
    'N', 'J', 'U', KEY_UP, '4', 'R', 'F', 'C',
    'M', 'K', 'I', KEY_RIGHT, '3', 'E', 'D', 'X',
    KEY_SYMBOL_SHIFT, 'L', 'O', '9', '2', 'W', 'S', 'Z',
    KEY_BREAK, KEY_ENTER, 'P', KEY_BACKSPACE, '1', 'Q', 'A' };

static char NORMAL_KEYS[39] = {
    'b', 'h', 'y', '6', '5', 't', 'g', 'v',
    'n', 'j', 'u', '7', '4', 'r', 'f', 'c',
    'm', 'k', 'i', '8', '3', 'e', 'd', 'x',
    KEY_SYMBOL_SHIFT, 'l', 'o', '9', '2', 'w', 's', 'z',
    ' ', KEY_ENTER, 'p', '0', '1', 'q', 'a'
};

static char SYM_KEYS[39] = {
    '*', KEY_UNKNOWN, '[', '&', '%', '>', '}', '/',
    ',', '-', ']', '\'', '$', '<', '{', '?',
    '.', '+', KEY_UNKNOWN, '(', '#', KEY_UNKNOWN, '\\', KEY_UNKNOWN,
    KEY_UNKNOWN, '=', ';', ')', '@', KEY_UNKNOWN, '|', ':',
    ' ', KEY_ENTER, '"', KEY_BACKSPACE, '!', KEY_UNKNOWN, '~'
};

char spectrum_kbd__to_ascii(uint16_t value)
{
    uint8_t modifier = value>>8;
    uint8_t mainkey = value & 0xff;
    const char *table = NULL;

    if (modifier==CAPS_SHIFT_CODE) {
        table = CAPSSHIFT_KEYS;
    } else if (modifier==SYMBOL_SHIFT_CODE) {
        table = SYM_KEYS;
    } else {
        table = NORMAL_KEYS;
    }
    if (mainkey>=sizeof(NORMAL_KEYS))
        return KEY_UNKNOWN;

    return table[mainkey];
}

