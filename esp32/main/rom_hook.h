#ifndef __ROM_HOOK_H__
#define __ROM_HOOK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include "model.h"

#define ROM_HOOK_FLAG_ACTIVE (1<<7)
#define ROM_HOOK_FLAG_SETRESET (1<<6)
#define ROM_HOOK_FLAG_PREPOST (1<<5)
#define ROM_HOOK_FLAG_RANGED  (1<<4)
#define ROM_HOOK_FLAG_ROM(x)  ((x)<<0)

int rom_hook__add(uint16_t start, uint8_t len, uint8_t flags);
void rom_hook__remove(int hook);

int rom_hook__enable_defaults();
void rom_hook__disable_defaults();



static inline int rom_hook__add_post_set_ranged(uint8_t rom, uint16_t start, uint8_t len);
static inline int rom_hook__add_pre_set_ranged(uint8_t rom, uint16_t start, uint8_t len);

static inline int rom_hook__add_post_set(uint8_t rom, uint16_t start, uint8_t len);
static inline int rom_hook__add_pre_set(uint8_t rom, uint16_t start, uint8_t len);
static inline int rom_hook__add_post_reset(uint8_t rom, uint16_t start, uint8_t len);
static inline int rom_hook__add_pre_reset(uint8_t rom, uint16_t start, uint8_t len);



static inline int rom_hook__add_post_set_ranged(uint8_t rom, uint16_t start, uint8_t len) {
    return rom_hook__add(start, len, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_SETRESET | ROM_HOOK_FLAG_RANGED | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_pre_set_ranged(uint8_t rom, uint16_t start, uint8_t len) {
    return rom_hook__add(start, len, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_PREPOST | ROM_HOOK_FLAG_SETRESET | ROM_HOOK_FLAG_RANGED | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_post_set(uint8_t rom, uint16_t start, uint8_t len)
{
    return rom_hook__add(start, len, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_SETRESET | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_pre_set(uint8_t rom, uint16_t start, uint8_t len)
{
    return rom_hook__add(start, len, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_PREPOST | ROM_HOOK_FLAG_SETRESET | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_post_reset(uint8_t rom, uint16_t start, uint8_t len)
{
    return rom_hook__add(start, len, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_ROM(rom));
}

static inline int rom_hook__add_pre_reset(uint8_t rom, uint16_t start, uint8_t len)
{
    return rom_hook__add(start, len, ROM_HOOK_FLAG_ACTIVE | ROM_HOOK_FLAG_PREPOST | ROM_HOOK_FLAG_ROM(rom));
}


#ifdef __cplusplus
}
#endif

#endif
