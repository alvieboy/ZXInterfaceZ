#include "joystick.h"
#include "kempston.h"
#include <string.h>

/**
 * \defgroup joystick
 * \brief Joystick routines
 */

static const struct {
    const char *name;
    joy_action_t action;
} joystick_actions[] = {
    { "up", JOY_UP },
    { "down", JOY_DOWN },
    { "left", JOY_LEFT },
    { "right", JOY_RIGHT },
    { "fire1", JOY_FIRE1 },
    { "fire2", JOY_FIRE2 },
    { "fire3", JOY_FIRE3 }
};

/**
 * \ingroup joystick
 * \brief Get a joystick action value by name
 *
 * \param name The action name
 * \return the action, or JOY_NONE if action not found
 */
joy_action_t joystick__get_action_by_name(const char *name)
{
    unsigned i;
    for(i=0;i<sizeof(joystick_actions)/sizeof(joystick_actions[0]); i++) {
        if (strcmp(name, joystick_actions[i].name)==0)
            return (int)joystick_actions[i].action;
    }
    return JOY_NONE;
}

/**
 * \ingroup joystick
 * \brief Activate a joystick movement
 */
void joystick__press(joy_action_t v)
{
    kempston__set_joystick(v, true );
}

/**
 * \ingroup joystick
 * \brief Deactivate a joystick movement
 */
void joystick__release(joy_action_t v)
{
    kempston__set_joystick(v, false );
}
