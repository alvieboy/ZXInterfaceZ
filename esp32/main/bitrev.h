#include <inttypes.h>

extern const uint8_t bitRevTable[256];

static inline uint8_t bitrev__byte(const uint8_t v)
{
    return bitRevTable[v];
}



