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

#include "uart.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"
#include "stm32f0xx_hal_gpio.h"
#include "defs.h"
#include <string.h>
#include <stdarg.h>
#include "vsnprintf.h"

static UART_HandleTypeDef UartHandle;

extern void Error_Handler();

void uart__init(void)
{
    UartHandle.Instance        = DEBUG_USARTx;

    UartHandle.Init.BaudRate   = 115200;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = UART_STOPBITS_1;
    UartHandle.Init.Parity     = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;

    if (HAL_UART_Init(&UartHandle) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }
}

void outbyte(int c)
{
    uint8_t ch = c;
    while (HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 0xFFFF)!=HAL_OK);
}

void outstring(const char *str)
{
    while (*str) {
        unsigned char c = *str;
        while (HAL_UART_Transmit(&UartHandle, &c, 1, 0xFFFF)!=HAL_OK);
        str++;
    }
}

int inbyte(unsigned char *c)
{
    if(HAL_UART_Receive(&UartHandle, c, 1, 0) != HAL_OK)
    {
        return -1;
    }
    return 0;

}


static void printnibble(unsigned int c)
{
    c&=0xf;
    if (c>9)
        outbyte(c+'a'-10);
    else
        outbyte(c+'0');
}

void printhexbyte(unsigned int c)
{
    printnibble(c>>4);
    printnibble(c);
}


void printhex(unsigned int c)
{
    printhexbyte(c>>24);
    printhexbyte(c>>16);
    printhexbyte(c>>8);
    printhexbyte(c);
}

void printhex16(unsigned short c)
{
    printhexbyte(c>>8);
    printhexbyte(c);
}

static char printf_line[128];

void uart__printf(const char *fmt,...)
{
    va_list ap;
    va_start(ap, fmt);
    int len = __vsnprintf(printf_line, 128, fmt, ap);
    if (len>0) {
        uart__puts(printf_line);
    }
    va_end(ap);
}

void uart__puts(const char *str)
{
    outstring(str);
}

