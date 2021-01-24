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

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "pin.h"
#include "debounce.h"

#define PINS_PER_JOYSTICK 9 /* DB9 */

// Pin indexes
#define KUP 0
#define KDOWN 1
#define KLEFT 2
#define KRIGHT 3
#define KFIRE 4
#define KBUTTON1 5
#define KBUTTON2 6
#define KBUTTON3 7

#define KPOWER (0xFD)
#define KGND (0xFE)
#define NC (0xFF)

#define IS_REGULAR_USAGE(pin) ((pin)<KPOWER)

typedef const uint8_t joystick_pin_usage_conf_t[PINS_PER_JOYSTICK];
typedef const pin_t joystick_pin_conf_t[PINS_PER_JOYSTICK];
typedef debounce_t joystick_debounce_t[PINS_PER_JOYSTICK];

struct joystick
{
    const joystick_pin_usage_conf_t *usage_conf;
    const joystick_pin_conf_t       *pin_conf;
    joystick_debounce_t debounce;
};

void joystick__init(struct joystick *joy,
                    const joystick_pin_conf_t *pin_conf);

void joystick__set_usage(struct joystick *joy,
                         const joystick_pin_usage_conf_t *usage);

bool joystick__scan_pins(struct joystick *joy);

void joystick__update(struct joystick *joy,
                      void (*updatefun)(void *,uint8_t usage, uint8_t val),
                      void*);


#endif
