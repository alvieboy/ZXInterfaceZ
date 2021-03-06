#ifndef __ROM_HOOK_H__
#define __ROM_HOOK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include "model.h"

#define ROM_HOOK_FLAG_ACTIVE (1<<7)
#define ROM_HOOK_FLAG_SETRESET (1<<6)
#define ROM_HOOK_FLAG_POST (1<<5)
#define ROM_HOOK_FLAG_RANGED  (1<<4)
#define ROM_HOOK_FLAG_ROM(x)  ((x)<<0)

typedef enum {
    MASK_LEN_1,
    MASK_LEN_2,
    MASK_LEN_8,
    MASK_LEN_256
} masklen_t;

int rom_hook__add(uint16_t start, masklen_t masklen, uint8_t flags);
void rom_hook__remove(int hook);

int rom_hook__enable_defaults();
void rom_hook__disable_defaults();


static inline int rom_hook__add_post_set_ranged(uint8_t rom, uint16_t start, masklen_t masklen);
static inline int rom_hook__add_pre_set_ranged(uint8_t rom, uint16_t start, masklen_t masklen);
static inline int rom_hook__add_post_set(uint8_t rom, uint16_t start, masklen_t masklen);
static inline int rom_hook__add_pre_set(uint8_t rom, uint16_t start, masklen_t masklen);
static inline int rom_hook__add_post_reset(uint8_t rom, uint16_t start, masklen_t masklen);
static inline int rom_hook__add_pre_reset(uint8_t rom, uint16_t start, masklen_t masklen);


static inline int rom_hook__add_post_set_ranged(uint8_t rom, uint16_t start, masklen_t masklen) {
    return rom_hook__add(start, masklen, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_POST | ROM_HOOK_FLAG_SETRESET | ROM_HOOK_FLAG_RANGED | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_pre_set_ranged(uint8_t rom, uint16_t start, masklen_t masklen) {
    return rom_hook__add(start, masklen, ROM_HOOK_FLAG_ACTIVE  | ROM_HOOK_FLAG_SETRESET | ROM_HOOK_FLAG_RANGED | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_post_set(uint8_t rom, uint16_t start, masklen_t masklen)
{
    return rom_hook__add(start, masklen, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_POST | ROM_HOOK_FLAG_SETRESET | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_pre_set(uint8_t rom, uint16_t start, masklen_t masklen)
{
    return rom_hook__add(start, masklen, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_SETRESET | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_post_reset(uint8_t rom, uint16_t start, masklen_t masklen)
{
    return rom_hook__add(start, masklen, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_POST  | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_pre_reset(uint8_t rom, uint16_t start, masklen_t masklen)
{
    return rom_hook__add(start, masklen, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_ROM(rom));
}
void rom_hook__dump();

#ifdef __cplusplus
}
#endif

#endif
