#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

/* Similar to Kempston sequence  000FUDLR */
typedef enum {
    RIGHT,
    LEFT,
    DOWN,
    UP,
    FIRE1,
    FIRE2
} joy_action_t;

int joystick__get_action_by_name(const char *name);
void joystick__press(int);
void joystick__release(int);

#endif
