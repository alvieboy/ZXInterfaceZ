#ifndef __GPIO_H__
#define __GPIO_H__

#include "driver/gpio.h"
#include "hal/gpio_ll.h"


void gpio__init();
int gpio__waitpin(gpio_num_t pin, uint8_t value, int timeout);

/* This needs to be here so that it can be fully inlined, because we need all code here to be
 in RAM */
static inline IRAM_ATTR int gpio__read(uint32_t gpio_num)
{
    return gpio_ll_get_level(GPIO_LL_GET_HW(GPIO_PORT_0), gpio_num);
}


#endif
