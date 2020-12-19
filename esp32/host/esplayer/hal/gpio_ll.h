#ifndef __HAL_GPIO_LL_H__
#define __HAL_GPIO_LL_H__

#define GPIO_LL_GET_HW(x) 0

extern uint64_t pinstate;

static inline int gpio_ll_get_level(void *data, uint32_t pin)
{
    return !!(pinstate & (1ULL<<pin));
}

static inline int gpio_ll_set_level(void *data, uint32_t pin, uint32_t value)
{
    if (value) {
        pinstate |= (1<<pin);
    } else {
        pinstate &= ~(1<<pin);
    }
}


#endif

