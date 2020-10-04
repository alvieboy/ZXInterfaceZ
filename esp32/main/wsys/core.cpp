#include "core.h"
#include "charmap.h"
#include "../fpga.h"
#include "pixel.h"
#include <stdarg.h>
#include <functional>
#include <map>
#include "systemevent.h"

extern "C" {
    struct framebuffer spectrum_framebuffer;
};

screenptr_t screenptr_t::drawchar(const uint8_t *data)
{
    screenptr_t temp = *this;
    uint16_t o = off;

    int i;
    for (i=0;i<8;i++) {
        CHECK_BOUNDS_SCREEN(o);
        spectrum_framebuffer.screen[o] = *data++;
        o+=256;
    }
    temp++;
    return temp;
}

void screenptr_t::nextpixelline()
{
    // Increment Y by 1 (Y0)
    off.h++;
    if ((off.h&0x7)==0) {
        // Overflow Y6.
        uint16_t l = off.l;
        l+=32;
        if (!(l&0x100)) {
            // Fix up, we added too much to Y6.
            off.h-=8;
        }
        off.l = l&0xff;
    }
}

void screenptr_t::nextcharline(int amt)
{
    // Increment Y by 8 (Y3)
    while (amt--) {
        uint16_t l = off.l;
        l += 32;
        if (l&0x100) {
            // Overflow.
            off.h += 0x08;
            off.h &= 0x1F;
        }
        off.l = l & 0xff;
    }
}

screenptr_t screenptr_t::drawascii(char c)
{
    screenptr_t temp = *this;
    if (c<FIRST_PRINTABLE_CHAR)
        return *this;
    c-=FIRST_PRINTABLE_CHAR;
    temp = temp.drawchar(&CHAR_SET[(int)c*8]);
    return temp;
}

screenptr_t screenptr_t::drawstring(const char *s)
{
    screenptr_t temp = *this;
    while (*s) {
        int c = (*s) - FIRST_PRINTABLE_CHAR;
        temp = temp.drawchar(&CHAR_SET[c*8])++;
        s++;
    }
    return temp;
}

screenptr_t drawthumbchar(screenptr_t screenptr, unsigned &bit_offset, char c)
{
    c-=32;
    int i;
    screenptr_t tmp = screenptr;

    uint8_t *charptr = &__tomthumb_bitmap__[c*6];

    for (i=0;i<6;i++) {
        pixel__draw8(tmp, bit_offset, 4, *charptr++);
        tmp.nextpixelline();
    }
    bit_offset+=4;
    if (bit_offset>7) {
        bit_offset-=8;
        screenptr++;
    }
    return screenptr;
}

screenptr_t drawthumbstring(screenptr_t screenptr, const char *s)
{
    unsigned off = 0;
    while (*s) {
        screenptr = drawthumbchar(screenptr, off, *s++);
    }
    return screenptr;
}



screenptr_t screenptr_t::drawstringpad(const char *s, int len)
{
    screenptr_t temp = *this;
    while ((*s) && len) {
        int c = (*s) - 32;
        temp = temp.drawchar(&CHAR_SET[c*8])++;
        s++;
        len--;
    }
    while (len--) {
        temp = temp.drawchar(&CHAR_SET[0])++;
    }
    return temp;
}

screenptr_t screenptr_t::drawhline(int len)
{
    screenptr_t temp = *this;
    while (len--) {
        CHECK_BOUNDS_SCREEN(off);
        spectrum_framebuffer.screen[off++] = 0xFF;
    }
    return temp;
}

screenptr_t screenptr_t::printf(const char *fmt,...)
{
    screenptr_t temp;
    va_list ap;
    char *c;
    va_start(ap, fmt);
    vasprintf(&c, fmt, ap);
    temp = drawstring(c);
    free(c);
    return temp;
}

static std::map<int,  std::function<void(const systemevent_t&)> > m_systemeventhandlers;
static int event_index = 0;

int wsys__subscribesystemeventfun(std::function<void(const systemevent_t&)> handler)
{
    m_systemeventhandlers[event_index] = handler;
    event_index++;
    return event_index-1;
}

void wsys__unsubscribesystemevent(int index)
{
    m_systemeventhandlers.erase( index );
}

void wsys__propagatesystemevent(const systemevent_t &event)
{
    for (auto i: m_systemeventhandlers) {
        (i.second)(event);
    }
}
