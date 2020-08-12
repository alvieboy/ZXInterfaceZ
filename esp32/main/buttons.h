#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include <inttypes.h>
#include <stdbool.h>

#define BUTTON_SWITCH 0
#define BUTTON_IO0    1

typedef enum {
    BUTTON_IDLE,
    BUTTON_PRESSED,
    BUTTON_LONG_PRESSED,
    BUTTON_RELEASED,
    BUTTON_LONG_RELEASED
} button_event_type_e;

typedef struct {
    uint8_t button;
    button_event_type_e type:8;
} button_event_t;

void buttons__init(void);
int buttons__getevent(button_event_t *event, bool block);

#endif

