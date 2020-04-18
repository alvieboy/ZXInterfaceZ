#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <stdbool.h>

void sdcard__init(void);
bool sdcard__isconnected(void);

#endif
