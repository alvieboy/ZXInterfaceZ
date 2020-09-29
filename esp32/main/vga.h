#ifndef __VGA_H__
#define __VGA_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <inttypes.h>

void vga__init(void);
int vga__setmode(uint8_t mode);
int vga__getmode(void);

#ifdef __cplusplus
}
#endif

#endif
