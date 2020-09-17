#ifndef __VGA_H__
#define __VGA_H__

#include <inttypes.h>

void vga__init(void);
int vga__setmode(uint8_t mode);
int vga__getmode(void);

#endif
