#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <inttypes.h>

void console__init(void);
void console__char(char c);
void console__hdlc_data(const uint8_t *d, unsigned len);

#endif
