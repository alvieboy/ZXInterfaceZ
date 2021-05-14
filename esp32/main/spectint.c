#include "os/queue.h"
#include "os/task.h"
#include "gpio.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "defs.h"
#include "spectint.h"
#include "hal/gpio_hal.h"

static Queue gpio_evt_queue = NULL;

static volatile uint32_t interrupt_count = 0;

static void IRAM_ATTR spectint__isr_handler(void* arg)
{
    int need_yield = 0;
    // This is done like this so it stays in IRAM
    gpio_hal_context_t _gpio_hal = {
        .dev = GPIO_HAL_GET_HW(GPIO_PORT_0)
    };

    gpio_hal_set_level(&_gpio_hal, PIN_NUM_INTACK, 0);
    // Small delay
    for (register int z=4; z!=0;--z) {
        __asm__ __volatile__ ("nop");
    }
    gpio_hal_set_level(&_gpio_hal, PIN_NUM_INTACK, 1);

    uint32_t gpio_num = ((uint32_t)(size_t) arg );
    while ( queue__send_from_isr(gpio_evt_queue, &gpio_num, &need_yield) != OS_TRUE);

    if (need_yield)
        task__yield_from_isr();
}

void spectint__init()
{
#ifdef __linux__
    gpio_evt_queue = queue__create(64, sizeof(uint32_t));
#else
    gpio_evt_queue = queue__create(8, sizeof(uint32_t));
#endif
    gpio_set_intr_type(PIN_NUM_CMD_INTERRUPT, GPIO_INTR_LOW_LEVEL);
    interrupt_count = 0;

    //install gpio isr service
    gpio_install_isr_service(0);
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_CMD_INTERRUPT, spectint__isr_handler, (void*) PIN_NUM_CMD_INTERRUPT));
}

int spectint__getinterrupt(int timeout)
{
    uint32_t io_num;
    if (queue__receive(gpio_evt_queue, &io_num, timeout)!=OS_TRUE)
        return 0;

    return io_num;
}
