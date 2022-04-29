#pragma once

#include "core.h"
#include "joystick.h"

class Window;
class Widget;

void screen__removeWindow(Window*);
void screen__destroyAll();
void screen__addWindow(Window*, uint8_t x, uint8_t y);
void screen__addWindowCentered(Window*);
void screen__redraw();
void screen__init();
void screen__keyboard_event(u16_8_t v);
void screen__joystick_event(joy_action_t action, bool on);
void screen__damage(Widget *source);
void screen__check_redraw();
void screen__grabKeyboardFocus(Widget *d);
void screen__releaseKeyboardFocus(Widget *d);
void screen__windowLoop(Window *w);
void screen__windowVisibilityChanged(Window *s, bool visible);
void screen__do_cleanup();
