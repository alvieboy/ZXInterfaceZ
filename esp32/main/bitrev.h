#include <inttypes.h>

extern const uint8_t bitRevTable[256];

/**
 \ingroup misc
 \brief Reverse all bits on a byte
 \param v byte to be reversed
 \return the byte with bits reversed
 */
static inline uint8_t bitrev__byte(const uint8_t v)
{
    return bitRevTable[v];
}



