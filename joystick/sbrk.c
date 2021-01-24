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
#include "sbrk.h"
#include "uart.h"
#include <stm32f0xx_hal.h>

#define MEM_PAGESIZE 4096

extern int __bss_end__;
//extern int __Stack_Init;

static void *mend = 0;

static void brk__initialise()
{
    unsigned ptr = (unsigned)&__bss_end__;
    ptr+=(MEM_PAGESIZE-1);
    ptr&=~(MEM_PAGESIZE-1);

    mend = (void*)ptr;
}

void *_sbrk(int increment)
{
    if (mend==0)
        brk__initialise();

    void *ret = (void*)mend;
    increment += (MEM_PAGESIZE-1);
    increment &= (~(MEM_PAGESIZE-1));

    void *n_mend=(void*)((unsigned char*)mend + increment);
#if 0
    if (n_mend>(void*)&__Stack_Init)  {
        outstring("******** OUT OF MEMORY *********");
        outstring("Requested: 0x");
        printhex(increment);
        outstring("\r\n  mend: 0x");
        printhex((uint32_t)mend);
        outstring("\r\n  StackInit: 0x");
        printhex((uint32_t)__Stack_Init);

        return 0;
    }
#endif
    mend = n_mend;
    return ret;
}
