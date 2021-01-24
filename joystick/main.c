/*
  ******************************************************************************
  * (C) 2021 Alvaro Lopes <alvieboy@alvie.com>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of Alvaro Lopes nor the names of contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include "main.h"
#include "setup.h"
#include "uart.h"
#include "defs.h"
#include "usbd.h"
#include <stdbool.h>
#include "vsnprintf.h"
#include "cpu.h"
#include "joystick.h"
#include "pin.h"
#include "array.h"

#define J1_1        MAKEPIN(PORT_B, 2)
#define J1_2        MAKEPIN(PORT_B, 11)
#define J1_3        MAKEPIN(PORT_B, 13)
#define J1_4        MAKEPIN(PORT_B, 15)
#define J1_5        MAKEPIN(PORT_A, 13)
#define J1_6        MAKEPIN(PORT_B, 10)
#define J1_7        MAKEPIN(PORT_B, 12)
#define J1_8        MAKEPIN(PORT_B, 14)
#define J1_9        MAKEPIN(PORT_A, 8)

#define J2_1        MAKEPIN(PORT_B, 5)
#define J2_2        MAKEPIN(PORT_B, 7)
#define J2_3        MAKEPIN(PORT_B, 9)
#define J2_4        MAKEPIN(PORT_A, 15)
#define J2_5        MAKEPIN(PORT_B, 4)
#define J2_6        MAKEPIN(PORT_B, 6)
#define J2_7        MAKEPIN(PORT_B, 8)
#define J2_8        MAKEPIN(PORT_A, 14)
#define J2_9        MAKEPIN(PORT_B, 3)

#define LED0        MAKEPIN(PORT_C, 15)
#define LED1        MAKEPIN(PORT_C, 14)
#define LED2        MAKEPIN(PORT_C, 13)


static const joystick_pin_conf_t joy0_pins = {
    J1_1, J1_2, J1_3, J1_4, J1_5, J1_6, J1_7, J1_8, J1_9
};

static const joystick_pin_conf_t joy1_pins = {
    J2_1, J2_2, J2_3, J2_4, J2_5, J2_6, J2_7, J2_8, J2_9
};

static joystick_pin_usage_conf_t atari_joy_conf = {
    KUP, KDOWN, KLEFT, KRIGHT, NC, KFIRE, KPOWER, KGND, KBUTTON1
};

#if 0

static joystick_pin_usage_conf_t kempston_joy_conf = {
    KUP, KDOWN, KLEFT, KRIGHT, KBUTTON2, KFIRE, KPOWER, KGND, KBUTTON1
};

static joystick_pin_usage_conf_t sjs1_joy_conf = {
    NC, KGND, NC, KFIRE, KUP, KRIGHT, KLEFT, KGND, KDOWN
};

#endif

static uint8_t report[2] = {0};
static struct joystick joysticks[2];

void tick__handler(void);


static void gpio__setup()
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_SPI1_CLK_DISABLE();
    __HAL_RCC_I2C1_CLK_DISABLE();

    __HAL_RCC_ADC1_CLK_DISABLE();

    pin__set_output(LED0, 1);
    pin__set_output(LED1, 1);
    pin__set_output(LED2, 1);

}
void tick__handler(void)
{
}

static void tx(uint8_t report, bool changed)
{
    if (usbd__hid_get_idle()==0 && !changed)
        return;
    usbd__sendreport(&report, 1);
}



static void update_joysticks()
{
    joystick__scan_pins(&joysticks[0]);
    joystick__scan_pins(&joysticks[1]);
}

static void update_joystick_report(void *rptptr,
                                   uint8_t usage,
                                   uint8_t val)
{
    uint8_t rpt = *(uint8_t*)rptptr;

    if (val==1)
        return; // Not toggled

    switch (usage) {
    case KLEFT:
        rpt |= (0x03)<<0; // -1
        break;
    case KRIGHT:
        rpt |= (0x01)<<0; // +1
        break;
    case KUP:
        rpt |= (0x03)<<2; // -1
        break;
    case KDOWN:
        rpt |= (0x01)<<2; // +1
        break;
    case KFIRE:
        rpt |= (0x01)<<4;
        break;
    case KBUTTON1:
        rpt |= (0x01)<<5;
        break;
    case KBUTTON2:
        rpt |= (0x01)<<6;
        break;
    default:
        break;
    }

    *(uint8_t*)rptptr = rpt;
}

static bool check_joysticks()
{
    uint8_t new_report[2] = {0};
    bool changed = false;

    // Generate reports based on pin settings.
    ARRAY_FOR_EACH(index, joysticks) {

        joystick__update(&joysticks[index],
                         &update_joystick_report,
                         &new_report[index]);

    }

    ARRAY_FOR_EACH(index, joysticks) {
        if (report[index]!=new_report[index]) {
            report[index] = new_report[index];
            changed = true;
        }
    }
    tx(report[0], changed);
    return true;
}

#include "timer.h"

static TimerEvent_t ledTimer;
static TimerEvent_t joystickTimer;

static void ledtimer_callback(void *user)
{
    static int led = 0;
    led = !led;
    pin__write(LED0, led);
}

static void joysticktimer_callback(void*user)
{
    check_joysticks();

    pin__write(LED1, !pin__read(J1_6));
    pin__write(LED2, !pin__read(J1_9));
}

int main()
{
    clk__setup();
    SystemCoreClockUpdate();
    HAL_Init();
    gpio__setup();

    joystick__init(&joysticks[0], &joy0_pins);
    joystick__init(&joysticks[1], &joy1_pins);

    joystick__set_usage(&joysticks[0], &atari_joy_conf);
    joystick__set_usage(&joysticks[1], &atari_joy_conf);

    usbd__init();

    TimerInit( &ledTimer, &ledtimer_callback, NULL);
    TimerSetPersistent( &ledTimer, 1);
    TimerSetValue( &ledTimer, 500);
    TimerStart( &ledTimer );

    TimerInit( &joystickTimer, &joysticktimer_callback, NULL);
    TimerSetPersistent( &joystickTimer, 1);
    TimerSetValue( &joystickTimer, 10);
    TimerStart( &joystickTimer );

    __enable_irq();


    while (1)
    {
        TimerLoop();

        update_joysticks();

        unsigned t = HAL_GetTick();
        while (t==HAL_GetTick()) {
            cpu__wait();
        }
    }
}

