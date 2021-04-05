#ifndef __BASICKEY_H__
#define __BASICKEY_H__

#include <stdbool.h>
#include <inttypes.h>


#ifdef __cplusplus
extern "C" {
#endif

bool basickey__has_inject(void);
uint8_t basickey__get_inject();
void basickey__clearinject(void);
void basickey__inject(const uint8_t *keys, unsigned len);
void basickey__set_callback( void(*callback)(void) );

#ifdef __cplusplus
}
#endif

#endif
