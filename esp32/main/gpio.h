#ifndef __GPIO_H__
#define __GPIO_H__

#include "driver/gpio.h"

void gpio__init();
int gpio__waitpin(gpio_num_t pin, uint8_t value, int timeout);

#endif
