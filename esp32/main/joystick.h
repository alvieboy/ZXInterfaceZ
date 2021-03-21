#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Similar to Kempston sequence  000FUDLR */
typedef enum {
    JOY_RIGHT,
    JOY_LEFT,
    JOY_DOWN,
    JOY_UP,
    JOY_FIRE1,
    JOY_FIRE2,
    JOY_FIRE3,
    JOY_NONE
} __attribute__((packed)) joy_action_t;

int joystick__get_action_by_name(const char *name);
void joystick__press(int);
void joystick__release(int);

#ifdef __cplusplus
}
#endif

#endif
