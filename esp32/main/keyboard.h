#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <inttypes.h>

void keyboard__init(void);
void keyboard__press(uint8_t key);
void keyboard__release(uint8_t key);

#endif
