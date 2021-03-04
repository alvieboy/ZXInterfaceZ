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

#include "vsnprintf.h"
#include <stdlib.h>
#include <ctype.h>
#include "fixed.h"

static inline int __isdigit(const char c)
{
    return (c>='0' && c<='9');
}

static inline int __atoi(const char *str)
{
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i) 
        res = res*10 + str[i] - '0'; 
    return res;
}

static inline size_t __my_utoa(unsigned long i, char**dest, int base, size_t size, int pad, int fill, int upper)
{
    char temp[65];
    char *str = &temp[sizeof(temp)];
    int s=0;
    
    if (size<1)
        return size;

    *str='\0';

    do {
        unsigned long m = i;
        i /= base;
        char c = m - base * i;
        *--str = c < 10 ? c + '0' : c + upper - 10;
        s++;
    } while(i);

    /* Copy back */
    while (s<pad) {
        *--str=fill; s++;
    }

    s=0;

    while (*str && (size!=0)) {
        **dest=*str++;
        size--,s++;
        (*dest)++;
    }
    return s;
}

int __snprintf(char *str, unsigned size, const char *format, ...)
{
    va_list ap;
    va_start(ap,format);
    int r = __vsnprintf(str,size,format,ap);
    va_end(ap);
    return r;
}

int __vsnprintf(char *str, unsigned size, const char *format, va_list ap)
{
    const char *start = str;
    char sizespec[4];
    int spptr=0;
    int isformat = 0;
    int upper;
    int base;
    int pad;
    int fill=0;

    size--;

    while (*format && size) {

        if (isformat) {
            base=10;
            upper='A';
            switch (*format) {
            case 'D':
            case 'd':
                do {
                    int n = va_arg(ap, int);
                    if (n<0) {
                        n=-n;
                        *str++='-', size--;
                    }
                    if (spptr) {
                        sizespec[spptr]='\0';
                        spptr=0;
                        pad=__atoi(sizespec);
                    } else {
                        pad=0;
                    }
                    size -= __my_utoa(n,&str,base,size,pad,fill,upper);
                } while (0);
                format++ , isformat=0;
                continue;
            case 'x':
                upper='a';
            case 'X':
                base=16;
            case 'U':
            case 'u':
                do {
                    unsigned n = va_arg(ap, unsigned);
                    if (spptr) {
                        sizespec[spptr]='\0';
                        spptr=0;
                        pad=__atoi(sizespec);
                    } else {
                        pad=0;
                    }
                    size -= __my_utoa(n,&str,base,size,pad,fill,upper);
                } while (0);
                format++ , isformat=0;
                continue;
            case 's':
                do {
                    const char *st = va_arg(ap,const char*);
                    /* To-do: padding */
                    while (size-- && *st) {
                        *str++=*st++;
                    }
                } while (0);
                break;
            case 'l':
                format++;
                continue;
            default:
                if (__isdigit(*format)) {
                    if ((*format)!='0' || spptr) {
                        /* Size specifier */
                        sizespec[spptr++]=*format;
                    }
                    if (spptr==0 && *format=='0') {
                        /* Padding char */
                        fill=*format;
                    }
                }
                else {
                    /* Unknown format, ignore */
                    format++, isformat=0;
                    continue;
                }
                format++;
                continue;
            }

            format++,isformat=0;
            continue;
        }
        if (*format=='%') {
            isformat=1, format++;
            continue;
        }

        fill=' ';

        /* Normal char */
        if (!size)
            break;
        size--,*str++=*format++;
    }

    *str='\0';

    return (str-start);
}

