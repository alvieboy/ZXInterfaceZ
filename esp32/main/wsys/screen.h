#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "core.h"

class Window;
class Widget;

void screen__removeWindow(Window*);
void screen__destroyAll();
void screen__addWindow(Window*, uint8_t x, uint8_t y);
void screen__addWindowCentered(Window*);
void screen__redraw();
void screen__init();
void screen__keyboard_event(u16_8_t v);
void screen__damage(Widget *source);
void screen__check_redraw();

#endif

