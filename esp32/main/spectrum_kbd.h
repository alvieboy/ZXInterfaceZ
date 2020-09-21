#include <inttypes.h>

#define CAPS_SHIFT_CODE (0x27)
#define SYMBOL_SHIFT_CODE (0x18)

#define KEY_SYMBOL_SHIFT (0x01)
#define KEY_CAPS_SHIFT (0x02)
#define KEY_BREAK (0x03)
#define KEY_ENTER (0x0D)
#define KEY_BACKSPACE (0x08)
#define KEY_UNKNOWN (0xFF)

#ifdef __cplusplus
extern "C" {
#endif

char spectrum_kbd__to_ascii(uint16_t value);

#ifdef __cplusplus
}
#endif
