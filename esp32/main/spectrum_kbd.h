#include <inttypes.h>

#define CAPS_SHIFT_CODE (0x27)
#define SYMBOL_SHIFT_CODE (0x18)

// Special keys, not necessarly ASCII
#define KEY_SYMBOL_SHIFT (0x01)
#define KEY_CAPS_SHIFT (0x02)
#define KEY_BREAK (0x03)
#define KEY_ENTER (0x0D)
#define KEY_BACKSPACE (0x08)

#define KEY_UP   (0xFB)
#define KEY_DOWN (0xFC)
#define KEY_LEFT (0xFD)
#define KEY_RIGHT (0xFE)
#define KEY_UNKNOWN (0xFF)

#ifdef __cplusplus
extern "C" {
#endif

unsigned char spectrum_kbd__to_ascii(uint16_t value);

#ifdef __cplusplus
}
#endif
