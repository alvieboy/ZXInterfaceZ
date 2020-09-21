#include "core.h"
#include "charmap.h"
#include "../fpga.h"

extern "C" {
    struct framebuffer spectrum_framebuffer;
};

screenptr_t &screenptr_t::drawchar(const uint8_t *data)
{
    uint16_t o = off;
    int i;
    for (i=0;i<8;i++) {
        CHECK_BOUNDS_SCREEN(o);
        spectrum_framebuffer.screen[o] = *data++;
        o+=256;
    }
    return *this;
}

/*      INC 	D				; Go down onto the next pixel line
	LD 	A, D				; Check if we have gone onto next character boundary
	AND	7
	RET 	NZ				; No, so skip the next bit
	LD 	A,E				; Go onto the next character line
	ADD 	A, 32
	LD 	E,A
	RET 	C				; Check if we have gone onto next third of screen
	LD 	A,D				; Yes, so go onto next third
	SUB 	8
	LD 	D,A
	RET
     */
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

void screenptr_t::nextcharline()
{
    // Increment Y by 8 (Y3)
    uint16_t l = off.l;
    l += 32;
    if (l&0x100) {
        // Overflow.
        off.h += 0x08;
        off.h &= 0x1F;
    }
    off.l = l & 0xff;
}

screenptr_t &screenptr_t::drawascii(char c)
{
    if (c<32)
        return *this;
    c-=32;
    drawchar(&CHAR_SET[(int)c*8]);
    return *this;
}
screenptr_t &screenptr_t::drawstring(const char *s)
{
    while (*s) {
        int c = (*s) - 32;
        ESP_LOGI("WSYS","Char 0x%02x %d", *s, c);
        drawchar(&CHAR_SET[c*8])++;
        s++;
    }
    return *this;
}

screenptr_t &screenptr_t::drawstringpad(const char *s, int len)
{
    while ((*s) && len) {
        int c = (*s) - 32;
        ESP_LOGI("WSYS","Char 0x%02x %d", *s, c);
        drawchar(&CHAR_SET[c*8])++;
        s++;
        len--;
    }
    while (len--) {
        drawchar(&CHAR_SET[0])++;
    }
    return *this;
}
screenptr_t &screenptr_t::drawhline(int len)
{
    while (len--) {
        CHECK_BOUNDS_SCREEN(off);
        spectrum_framebuffer.screen[off++] = 0xFF;
    }
    return *this;
}


