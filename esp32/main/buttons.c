/**
 \defgroup buttons Button handling
 \brief Routines to handle the board buttons
 */
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "buttons.h"
#include "defs.h"
#include "gpio.h"

#define BUTTON_TIMER TIMER_0

#define TIMER_DIVIDER         16  //  Hardware timer clock divider


static uint8_t button_counters[2];

#define DEBOUNCE_MAX 10
static uint8_t debounce[2];

#define PRESS_SHORT 8
#define PRESS_LONG 254
#define PRESS_MAX  255

static xQueueHandle button_event_queue;

static inline void IRAM_ATTR button__check(int index, uint32_t gpio_num)
{
    // Debounce button
    button_event_t event;
    event.button = index;
    event.type = BUTTON_IDLE;

    // ALL CALLS need to be on IRAM, because we might be accessing the flash.

    int v = gpio__read(gpio_num);
    if (v==0) {
        if (debounce[index]>0)
            debounce[index]--;
    } else {
        if (debounce[index]<DEBOUNCE_MAX)
            debounce[index]++;
    }
    if (debounce[index]==0) {
        // Button fully down
        if (button_counters[index]<PRESS_MAX)
            button_counters[index]++;

        if (button_counters[index] == PRESS_LONG) {
            // Emit short press
            event.type = BUTTON_LONG_PRESSED;
        }

        if (button_counters[index] == PRESS_SHORT) {
            // Emit long press
            event.type = BUTTON_PRESSED;
        }
    }

    if (debounce[index]==DEBOUNCE_MAX) {
        // Button fully up
        if (button_counters[index]!=0) {
            // Release event
            if (button_counters[index]>=PRESS_LONG) {
                event.type = BUTTON_LONG_RELEASED;
            } else {
                event.type = BUTTON_RELEASED;
            }
            button_counters[index]=0;
        }
    }
    if (event.type!=BUTTON_IDLE) {
        xQueueSendFromISR(button_event_queue, &event, 0);
    }
}

void IRAM_ATTR button_check_isr(void *para)
{
//    timer_spinlock_take(TIMER_GROUP_0);
    button__check(0, PIN_NUM_SWITCH);
    button__check(1, PIN_NUM_IO0);
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, BUTTON_TIMER);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, BUTTON_TIMER);
//    timer_spinlock_give(TIMER_GROUP_0);
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */

/**
 \ingroup buttons
 \brief Initialize the button subsystem

 This should be called only once during system startup.
 */
void buttons__init(void)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
    }; // default clock source is APB
    timer_init(TIMER_GROUP_0, BUTTON_TIMER, &config);

    timer_set_counter_value(TIMER_GROUP_0, BUTTON_TIMER, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, BUTTON_TIMER, TIMER_BASE_CLK/(500*16));

    timer_enable_intr(TIMER_GROUP_0, BUTTON_TIMER);
    timer_isr_register(TIMER_GROUP_0, BUTTON_TIMER, button_check_isr,
                       (void *)BUTTON_TIMER, ESP_INTR_FLAG_IRAM, NULL);


    button_event_queue = xQueueCreate(4, sizeof(button_event_t));

    debounce[0] = 3;
    debounce[1] = 3;
    button_counters[0] = 0;
    button_counters[1] = 0;

    timer_start(TIMER_GROUP_0, BUTTON_TIMER);
}

/**
 \ingroup buttons
 \brief Retrieve button event, if present

 If block is true, then wait indefenitely for the button event.

 The button propagation is done using an event queue, so only one task should
 retrieve events.

 \param event Pointer to location where to store the button event
 \param block Wait indefinitely (block) for a button event.
 \return -1 if no button event is present, 0 otherwise
 */
int buttons__getevent(button_event_t *event, bool block)
{
    if (!xQueueReceive(button_event_queue, event, block? portMAX_DELAY:0))
        return -1;
    return 0;
}
