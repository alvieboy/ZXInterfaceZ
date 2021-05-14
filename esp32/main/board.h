#ifndef __BOARD_H__
#define __BOARD_H__

#ifdef __cplusplus
extern "C"  {
#endif

#include <stdbool.h>
#include <inttypes.h>

void board__init(void);
bool board__is5Vsupply(void);
bool board__is9Vsupply(void);
bool board__is12Vsupply(void);
bool board__isCompatible(uint8_t major, uint8_t minor);

static inline bool board__isplus2plus3supply(void)
{
    return board__is5Vsupply() || board__is12Vsupply();
}

bool board__hasVoltageSensor(void);

int board__parseRevision(const char *str, uint8_t *major, uint8_t *minor);

#ifdef __cplusplus
}
#endif

#endif
