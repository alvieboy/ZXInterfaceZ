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

#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stm32f0xx_hal.h>


typedef uint32_t TimerTime_t;
typedef void (*TimerCallback_t)(void*);


typedef struct TimerEvent {
    uint32_t expire;
    uint32_t current;
    uint32_t requested;
    TimerCallback_t cb;
    void *userdata;
    uint8_t enabled:1;
    uint8_t persistent:1;
    uint8_t accurate:1;
} TimerEvent_t;


void TimerStop( TimerEvent_t * );
void TimerInit( TimerEvent_t*, TimerCallback_t cb, void *data);
uint32_t TimerGetCurrentTime(void);
uint32_t TimerGetElapsedTime(uint32_t start);
void TimerSetValue( TimerEvent_t *, uint32_t );
void TimerStart( TimerEvent_t*);
void TimerSetPersistent( TimerEvent_t *ev, uint8_t val);
bool TimerIsStarted( TimerEvent_t*);

void TimerLoop(void);
unsigned TimerCount(void);

#ifdef __cplusplus
}
#endif

#endif
