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

#ifndef __PIN_H__
#define __PIN_H__

#include <inttypes.h>

typedef uint8_t pin_t;

#ifndef __linux__

#include <stm32f0xx_hal.h>
#include <stm32f0xx_hal_gpio.h>

#define PORT_A 0
#define PORT_B 1
#define PORT_C 2
#define PORT_D 3

#define MAKEPIN(port, pin) ((pin|(port<<6)))


static inline GPIO_TypeDef *pin__getport(pin_t pin);
static inline uint32_t pin__getpin(pin_t pin);

static inline void pin__set_input(pin_t pin);
static inline int pin__read(pin_t pin);
static inline void pin__write(pin_t pin, int val);
static inline void pin__set_output(pin_t pin, int value);

/* Static inline functions */

static inline GPIO_TypeDef *pin__getport(pin_t pin)
{
    switch(pin>>6) {
    case 0: return GPIOA;
    case 1: return GPIOB;
    case 2: return GPIOC;
    case 3: return GPIOD;
    }
    return NULL;
}

static inline uint32_t pin__getpin(pin_t pin)
{
    return (1<<(pin&0x1F));
}

static inline void pin__set_input(pin_t pin)
{
    GPIO_InitTypeDef init;
    init.Mode = GPIO_MODE_INPUT;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    init.Pin = pin__getpin(pin);

    GPIO_TypeDef *port = pin__getport(pin);
    HAL_GPIO_Init( port, &init );
}

static inline void pin__set_output(pin_t pin, int value)
{
    GPIO_InitTypeDef init;
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    init.Pin = pin__getpin(pin);

    GPIO_TypeDef *port = pin__getport(pin);
    HAL_GPIO_Init( port, &init );
    pin__write(pin, value);
}

static inline int pin__read(pin_t pin)
{
    GPIO_TypeDef *port = pin__getport(pin);
    return HAL_GPIO_ReadPin(port, pin__getpin(pin));
}

static inline void pin__write(pin_t pin, int val)
{
    GPIO_TypeDef *port = pin__getport(pin);
    return HAL_GPIO_WritePin(port, pin__getpin(pin), val);

}

#else
void pin__set_input(pin_t pin);
int pin__read(pin_t pin);
void pin__write(pin_t pin, int val);
void pin__set_output(pin_t pin, int value);
#endif

#endif
