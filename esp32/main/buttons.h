#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include <inttypes.h>
#include <stdbool.h>

#define BUTTON_SWITCH 0 /** USR button */
#define BUTTON_IO0    1 /** IO0 button */

/**
 * \ingroup buttons
 * \brief Event type
 */
typedef enum {
    BUTTON_IDLE /** No button pressed */,
    BUTTON_PRESSED /** Button pressed */,
    BUTTON_LONG_PRESSED /** Button long pressed */,
    BUTTON_RELEASED /** Button released */,
    BUTTON_LONG_RELEASED /** Button released after a long press */
} button_event_type_e;

/**
 * \ingroup buttons
 * \brief Event
 */
typedef struct {
    /** \brief the button it refers to. See button macros */
    uint8_t button;
    /** \brief the button event type */
    button_event_type_e type:8;
} button_event_t;

void buttons__init(void);
int buttons__getevent(button_event_t *event, bool block);

#endif

