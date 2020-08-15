#define GPIO_LL_GET_HW(x) 0

extern uint64_t pinstate;

static inline int gpio_ll_get_level(void *data, uint32_t pin)
{
    return !!(pinstate & (1ULL<<pin));
}


