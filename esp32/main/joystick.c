#include "joystick.h"
#include "kempston.h"
#include <string.h>

static const struct {
    const char *name;
    joy_action_t action;
} joystick_actions[] = {
    { "up", UP },
    { "down", DOWN },
    { "left", LEFT },
    { "right", RIGHT },
    { "fire1", FIRE1 },
    { "fire2", FIRE2 }
};

int joystick__get_action_by_name(const char *name)
{
    unsigned i;
    for(i=0;i<sizeof(joystick_actions)/sizeof(joystick_actions[0]); i++) {
        if (strcmp(name, joystick_actions[i].name)==0)
            return (int)joystick_actions[i].action;
    }
    return -1;
}

void joystick__press(int v)
{
    kempston__set_joystick( (joy_action_t)v, true );
}

void joystick__release(int v)
{
    kempston__set_joystick( (joy_action_t)v, false );
}
