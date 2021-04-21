#ifndef __RESET_H__
#define __RESET_H__

#include <inttypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int reset__reset_spectrum(void);
int reset__reset_to_custom_rom(int romno, uint8_t miscctrl, bool activate_retn_hook);
int reset__get_reset_time(void);

#ifdef __cplusplus
}
#endif

#endif
