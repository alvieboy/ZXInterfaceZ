#include "log.h"
#include <stdarg.h>

static char *printnibble(char *dest, unsigned int c)
{
    c&=0xf;
    if (c>9)
        *dest = c+'a'-10;
    else
        *dest = c+'0';
    dest++;
    return dest;
}

static char *printhexbyte(char *dest, unsigned int c)
{
    dest = printnibble(dest, c>>4);
    dest = printnibble(dest, c);
    return dest;
}

void log_buffer(esp_log_level_t level, const char *tag, const char *hdr,const uint8_t *buf, uint8_t len)
{
    va_list ap;
    char *wbuf = malloc(1+len*3);
    char *wbufp = wbuf;
    if (!wbuf)
        return;

    while (len--) {
        wbufp = printhexbyte(wbufp, *buf++);
        *wbufp++=' ';
    }
    *wbufp = '\0';
    esp_log_write(level,
                   tag,
                  "%s: %s\n",
                  hdr,
                   wbuf);
    free(wbuf);
}

