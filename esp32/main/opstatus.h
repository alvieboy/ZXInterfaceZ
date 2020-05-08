#ifndef __OPSTATUS_H__
#define __OPSTATUS_H__

#include <inttypes.h>

#define OPSTATUS_IN_PROGRESS (0xfe)
#define OPSTATUS_ERROR (0xff)
#define OPSTATUS_OK (0x00)

void opstatus__set_status(uint8_t val, const char *str);
void opstatus__set_error(const char *str);

#endif
