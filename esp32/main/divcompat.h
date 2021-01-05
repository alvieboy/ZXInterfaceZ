#ifndef __DIVCOMPAT_H__
#define __DIVCOMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

int divcompat__enable(uint8_t rom);
void divcompat__disable(void);

#ifdef __cplusplus
}
#endif

#endif
