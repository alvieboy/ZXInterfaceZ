#include "gpio.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "defs.h"

void gpio__init()
{
    gpio_config_t io_conf;

    gpio_set_level(PIN_NUM_NCONFIG, 1);

    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT_OD;
    io_conf.pin_bit_mask = (1ULL<<PIN_NUM_LED1) | (1ULL<<PIN_NUM_LED2)
        | (1ULL<<PIN_NUM_NCONFIG);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en   = 0;

    gpio_config(&io_conf);


    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        (1ULL<<PIN_NUM_AS_CSN) |
        (1ULL<<PIN_NUM_CS);

    gpio_config(&io_conf);

    gpio_set_level(PIN_NUM_AS_CSN, 1);
    gpio_set_level(PIN_NUM_CS, 1);


    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
        (1ULL<<PIN_NUM_SWITCH) |
        (1ULL<<PIN_NUM_CONF_DONE) |
        (1ULL<<PIN_NUM_IO25) |
        (1ULL<<PIN_NUM_SPECT_INTERRUPT) |
        (1ULL<<PIN_NUM_IO14);

    io_conf.pull_up_en   = 1;

    gpio_config(&io_conf);
}
