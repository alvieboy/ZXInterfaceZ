/*
  ******************************************************************************
  * (C) 2018 Alvaro Lopes <alvieboy@alvie.com>
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

#include "usbd.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "delay.h"
#include "uart.h"
#include "ringbuffer.h"
#include "vsnprintf.h"
#include "usbhid.h"

USBD_HandleTypeDef  USBD_Device;

extern USBD_DescriptorsTypeDef VCP_Desc;

#if 0
static void usbd__forcereset(void)
{
    GPIO_InitTypeDef init;

    HAL_GPIO_WritePin( GPIOA, GPIO_PIN_11, 0);
    HAL_GPIO_WritePin( GPIOA, GPIO_PIN_12, 0);

    init.Pin = GPIO_PIN_11|GPIO_PIN_12;
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init( GPIOA, &init );
    HAL_Delay(200);
    init.Pin = GPIO_PIN_11|GPIO_PIN_12;
    init.Mode = GPIO_MODE_INPUT;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init( GPIOA, &init );
}
#endif

extern USBD_DescriptorsTypeDef Desc;

void usbd__init(void)
{
#if 0
    usbd__forcereset();
#endif
    usbhid__begin(&USBD_Device, &Desc);
}

int usbd__sendreport(uint8_t id,
                     uint8_t *report,
                     uint16_t len)
{
    return usbhid__sendreport(&USBD_Device, id, report, len);
}

int usbd__hid_get_idle()
{
    return usbhid__get_idle(&USBD_Device);
}
