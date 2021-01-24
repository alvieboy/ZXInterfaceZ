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

#include <stm32f0xx_hal.h>
#include <stm32f0xx_hal_tim.h>
#include <stm32f0xx_hal_rcc.h>
#include "defs.h"
#include "uart.h"
#include "delay.h"

TIM_HandleTypeDef    TimHandle;

#define DELAY_TIMx                            TIM2    /* Caution: Timer instance must be on APB1 (clocked by PCLK1) due to frequency computation in function "TIM_Config()" */
#define DELAY_TIMx_CLK_ENABLE()               __HAL_RCC_TIM2_CLK_ENABLE()
        
#define DELAY_TIMx_FORCE_RESET()              __HAL_RCC_TIM2_FORCE_RESET()
#define DELAY_TIMx_RELEASE_RESET()            __HAL_RCC_TIM2_RELEASE_RESET()

void delay__init(void)
{
    DELAY_TIMx_CLK_ENABLE();

    TimHandle.Instance = DELAY_TIMx;
    TimHandle.Init.Period            = 0xFFFF;
    TimHandle.Init.Prescaler         = ((36*2) - 1);
    TimHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TimHandle.Init.RepetitionCounter = 0x0;

    if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
    {
        /* Timer initialization Error */
        Error_Handler();
    }

    HAL_TIM_Base_Start(&TimHandle);
}

static uint16_t delay__read()
{
    uint16_t v = DELAY_TIMx->CNT;
    return v;
}

void delay__us(uint16_t us)
{
    DELAY_TIMx->CNT = 0;
    int16_t expire = (int16_t)(delay__read() + us);
    while (( expire - (int16_t)delay__read() ) > 0);
}

