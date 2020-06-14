#include "driver/gpio.h"
#include <malloc.h>

struct gpioh {
    struct gpioh *next;
    gpio_num_t num;
    gpio_isr_t isr;
    void *args;
};

static struct gpioh *handlers = NULL;

esp_err_t gpio_config(const gpio_config_t *pGPIOConfig)
{
    return ESP_OK;
}

esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type)
{
    return ESP_OK;
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args)
{
    struct gpioh *h = malloc(sizeof(struct gpioh));
    h->next = handlers;
    h->num = gpio_num;
    h->isr = isr_handler;
    h->args = args;
    handlers = h;

    return ESP_OK;
}

esp_err_t gpio_install_isr_service(int intr_alloc_flags)
{
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    return 0;
}

void gpio_isr_do_trigger(gpio_num_t num)
{
    struct gpioh *h = handlers;
    while (h) {
        if (h->num == num) {
            h->isr(h->args);
        }
        h = h->next;
    }
}
