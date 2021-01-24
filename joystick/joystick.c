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

#include <stdlib.h>
#include "joystick.h"
#include "array.h"
#include <stdio.h>

static void joystick__configure_pins(const struct joystick *joy)
{
    ARRAY_FOR_EACH(i, *joy->pin_conf) {
        pin_t pin = (*joy->pin_conf)[i];

        if (joy->usage_conf==NULL) {
            pin__set_input( pin );
            continue;
        }

        switch ((*joy->usage_conf)[i]) {
        case KGND:
            pin__set_output( pin, 0);
            break;
        case NC:
            pin__set_input( pin );
            break;
        default:

            pin__set_input( pin );
            break;
        }
    }
}

void joystick__init(struct joystick *joy,
                    const joystick_pin_conf_t *pin_conf)
{
    joy->pin_conf = pin_conf;
    joy->usage_conf = NULL;
    ARRAY_FOR_EACH(i, joy->debounce) {
        debounce__init( &joy->debounce[i] );
    }
    joystick__configure_pins(joy);
}

void joystick__set_usage(struct joystick *joy,
                         const joystick_pin_usage_conf_t *usage)
{
    joy->usage_conf = usage;

    joystick__configure_pins(joy);
}

bool joystick__scan_pins(struct joystick *joy)
{
    bool changed = false;
    if (joy->usage_conf==NULL)
        return false;

    ARRAY_FOR_EACH(i, *joy->pin_conf) {
        uint8_t usage = (*joy->usage_conf)[i];
        if (IS_REGULAR_USAGE(usage)) {
            if (debounce__read_pin(&joy->debounce[i], (*joy->pin_conf)[i]))
                changed = true;
        }
    }
    return changed;
}

void joystick__update(struct joystick *joy,
                      void (*updatefun)(void *,uint8_t usage, uint8_t val),
                      void*user)
{
    ARRAY_FOR_EACH(i, *joy->pin_conf) {
        uint8_t usage = (*joy->usage_conf)[i];
        if (IS_REGULAR_USAGE(usage)) {
            updatefun(user, usage, joy->debounce[i].value);
        }
    }
}