#include "led.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "defs.h"

void led__set(led_t led, uint8_t on)
{
    gpio_num_t gpio;
    if (led==LED1) {
        gpio = PIN_NUM_LED1;
    } else {
        return;// -1;//gpio = PIN_NUM_LED2;
    }
    gpio_set_level(gpio, !on);
}
