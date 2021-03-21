#ifndef __UNIONTYPES_H__
#define __UNIONTYPES_H__

#include "inttypes.h"

union u32 {
    uint32_t w32;
    uint8_t w8[4];
    uint16_t w16[2];
};

#endif
