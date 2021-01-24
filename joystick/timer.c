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

#include "timer.h"
#include <inttypes.h>
#include <stdbool.h>
#include "defs.h"

#define MAX_TIMERS 16

static TimerEvent_t *activetimers[MAX_TIMERS] = {0};

static uint32_t WallTime()
{
    return HAL_GetTick();
}

void TimerStop( TimerEvent_t *ev )
{
    unsigned i;
    ev->enabled=0;
    for (i=0;i<MAX_TIMERS;i++) {
        if (activetimers[i] == ev) {
            activetimers[i] = NULL;
            break;
        }
    }
}

void TimerInit( TimerEvent_t*ev, TimerCallback_t cb, void *data)
{
    ev->cb = cb;
    ev->enabled=0;
    ev->persistent=0;
    ev->accurate=0;
    ev->userdata = data;
}

uint32_t TimerGetElapsedTime(uint32_t start)
{
    uint32_t d = WallTime() - start;
    return d;
}

void TimerSetPersistent( TimerEvent_t *ev, uint8_t val)
{
    ev->persistent = val;
}

void TimerSetValue( TimerEvent_t *ev, uint32_t val)
{
    ev->requested = val;
}

void TimerStart( TimerEvent_t *ev)
{
    unsigned i;
    ev->expire = WallTime() + ev->requested;

    for (i=0;i<MAX_TIMERS;i++) {
        if (activetimers[i] == NULL) {
            activetimers[i] = ev;
            ev->enabled = 1;
            break;
        }
    }
    if (i==MAX_TIMERS) {
        Error_Handler();
    }
}


uint32_t TimerGetCurrentTime(void) {
    return WallTime();
}

bool TimerIsStarted( TimerEvent_t*ev)
{
    return ev->enabled;
}

static void TimerCheck(TimerEvent_t **element, uint32_t tick)
{
    TimerEvent_t *timer = (*element);

    if (timer==NULL) {
        return;
    }

    if (timer->enabled && timer->expire <= tick) {

        if (timer->cb) {
            timer->cb(timer);
        }
        if (timer->enabled) {
            if (!timer->persistent) {
                timer->enabled = 0;
                (*element) = NULL;
            } else {
                if (timer->accurate) {
                    timer->expire += timer->requested;
                } else {
                    timer->expire = WallTime()+timer->requested;
                }
            }
        }
    }
}

void TimerLoop(void)
{
    unsigned i;
    uint32_t tick = WallTime();
    for (i=0;i<MAX_TIMERS;i++) {
        if (activetimers[i]!=NULL) {
            TimerCheck(&activetimers[i], tick);
        }
    }
}

unsigned TimerCount(void)
{
    unsigned count = 0;
    unsigned i;
    for (i=0;i<MAX_TIMERS;i++) {
        if (activetimers[i]!=NULL) {
            count++;
        }
    }
    return count;
}
