#include "core.h"
#include "tomthumb.h"

static inline void pixel__draw8(screenptr_t screenptr, unsigned bit_offset, unsigned width, uint8_t val)
{
    if (bit_offset==0) {
        *screenptr = val;
        return;
    } else {
        uint16_t dpix = ((uint16_t)screenptr[0])<<8;
        dpix += (uint16_t)screenptr[1];
        uint16_t mask = ((1U<<width)-1) <<(8- bit_offset);
        dpix &= ~mask;
        dpix |= ((uint16_t)val)<<(8-bit_offset);
        screenptr[0] = dpix>>8;
        screenptr[1] = dpix&0xff;
    }
}

static inline void pixel__draw8xor(screenptr_t screenptr, unsigned bit_offset, unsigned width, uint8_t val)
{
    if (bit_offset==0) {
        *screenptr ^= val;
        return;
    } else {
        uint16_t dpix = ((uint16_t)screenptr[0])<<8;
        dpix += (uint16_t)screenptr[1];
        uint16_t mask = ((1U<<width)-1) <<(8- bit_offset);
        //dpix &= ~mask;
        dpix ^= ((uint16_t)val)<<(8-bit_offset);
        screenptr[0] = dpix>>8;
        screenptr[1] = dpix&0xff;
    }
}

