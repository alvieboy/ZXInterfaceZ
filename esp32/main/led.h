#ifndef __LED_H__
#define __LED_H__

#include <inttypes.h>

typedef enum {
    LED1,
    LED2
} led_t;

void led__set(led_t led, uint8_t on);

#endif
