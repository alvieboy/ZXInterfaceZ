#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup joystick
 * \brief Joystick actions
 *
 * Similar to Kempston sequence  f00FUDLR
 *
 */
typedef enum {
    JOY_RIGHT /** Joystick right */,
    JOY_LEFT /** Joystick left */,
    JOY_DOWN /** Joystick down */,
    JOY_UP /** Joystick up */,
    JOY_FIRE1 /** Joystick fire button 1 */,
    JOY_FIRE2 /** Joystick fire button 2 */,
    JOY_FIRE3 /** Joystick fire button 3 */,
    JOY_NONE  /** No event */
} __attribute__((packed)) joy_action_t;

joy_action_t joystick__get_action_by_name(const char *name);
void joystick__press(joy_action_t);
void joystick__release(joy_action_t);

#ifdef __cplusplus
}
#endif

#endif
