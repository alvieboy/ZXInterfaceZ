#ifndef __KEMPSTON_H__
#define __KEMPSTON_H__

#include "joystick.h"
#include <stdbool.h>
#include <inttypes.h>

void kempston__init();
void kempston__set_mouse(uint8_t x, uint8_t y, uint8_t button1, uint8_t button2);
void kempston__set_joystick_raw(uint8_t val);
void kempston__set_joystick(joy_action_t axis, bool on);

#endif

