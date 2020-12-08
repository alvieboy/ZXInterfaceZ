#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "gpio.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "defs.h"
#include "spectint.h"

static xQueueHandle gpio_evt_queue = NULL;

static volatile uint32_t interrupt_count = 0;

static void IRAM_ATTR spectint__isr_handler(void* arg)
{
    uint32_t gpio_num = ((uint32_t)(size_t) arg );
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    interrupt_count++;
}

extern void usb__isr_handler(void*);

void spectint__init()
{
    gpio_evt_queue = xQueueCreate(8, sizeof(uint32_t));

    gpio_set_intr_type(PIN_NUM_SPECT_INTERRUPT, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(PIN_NUM_CMD_INTERRUPT, GPIO_INTR_NEGEDGE);
    //gpio_set_intr_type(PIN_NUM_CMD_INTERRUPT, GPIO_INTR_LOW_LEVEL);
    gpio_set_intr_type(PIN_NUM_USB_INTERRUPT, GPIO_INTR_NEGEDGE);
    interrupt_count = 0;

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_SPECT_INTERRUPT, spectint__isr_handler, (void*) PIN_NUM_SPECT_INTERRUPT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_CMD_INTERRUPT, spectint__isr_handler, (void*) PIN_NUM_CMD_INTERRUPT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_USB_INTERRUPT, usb__isr_handler, (void*) PIN_NUM_USB_INTERRUPT));
}

int spectint__getinterrupt()
{
    uint32_t io_num;
    if (!xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        return 0;

    return io_num;
}
