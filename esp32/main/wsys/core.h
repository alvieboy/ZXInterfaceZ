#ifndef __WSYS_CORE_H__
#define __WSYS_CORE_H__

#include <inttypes.h>
#include <stdlib.h>
#include "../wsys.h"
extern "C" {
#include "esp_log.h"
};


struct framebuffer {
    uint8_t screen[32*24*8];
    uint8_t attr[32*24];
    uint8_t seq;
} __attribute__((packed));

#define CHECK_BOUNDS_SCREEN(x) do { \
    if (off.v>sizeof(spectrum_framebuffer.screen)) { \
    ESP_LOGE("WSYS", "OUT of bounds screen acccess %d (%d)", off.v, sizeof(spectrum_framebuffer.screen));\
    abort(); \
    } \
} while (0);
#define CHECK_BOUNDS_ATTR(x) do { \
    if (off>sizeof(spectrum_framebuffer.attr)) { \
    ESP_LOGE("WSYS", "OUT of bounds attr acccess %d (%d)", off, sizeof(spectrum_framebuffer.attr));\
    abort(); \
    }\
} while (0);

extern "C" struct framebuffer spectrum_framebuffer;


static inline uint16_t getxyattrstart(uint8_t x, uint8_t y)
{
    uint16_t off = (x+(y*32));
    return off;
}

union u16_8_t
{
    struct {
        uint8_t l;
        uint8_t h;
    };
    uint16_t v;
    operator uint16_t () const { return v; }
    u16_8_t operator++(int) {  u16_8_t s = *this; v++; return s; }
    u16_8_t &operator+=(int delta) { v +=delta; return *this; }
};


static inline u16_8_t getxyscreenstart(uint8_t x, uint8_t y)
{
    u16_8_t off;
    off.l = (y<<5) & 0xE0;
    off.l |= x;
    off.h = y & 0x18;
    return off;
    /*GETXYSCREENSTART:
        LD	A, D
	RRCA			; multiply
	RRCA			; by
	RRCA			; thirty-two.
	AND	$E0		; mask off low bits to make
	ADD     A, C
        LD      L,A

	LD	A,D		; bring back the line to A.
	AND	$18		; now $00, $08 or $10.
        OR	$40		; add the base address of screen.
	LD      H,  A
        RET
    */
    return off;
}


struct attrptr_t {

    void fromxy(uint8_t x, uint8_t y) {
        off = getxyattrstart(x,y);
    }
    void nextline() { off+=32; }
    attrptr_t& operator++() { off++; return *this; }
    attrptr_t operator++(int delta __attribute__((unused))) { attrptr_t s = *this; off++; return s; }
    attrptr_t &operator+=(int delta) { off +=delta; return *this; }
    uint8_t & operator*() {
        CHECK_BOUNDS_ATTR(off);
        return spectrum_framebuffer.attr[off];
    }
    uint16_t getoff() const { return off; }
private:
    uint16_t off;
};

struct screenptr_t {
    void fromxy(uint8_t x, uint8_t y) {
        off.v = getxyscreenstart(x,y);
    }
    void nextpixelline();
    void nextcharline();
    screenptr_t &drawchar(const uint8_t *data);
    screenptr_t &drawascii(char);
    screenptr_t &drawstring(const char *);
    screenptr_t &drawstringpad(const char *, int len);
    screenptr_t &drawhline(int len);
    screenptr_t &operator++() { off++; return *this; }
    screenptr_t operator++(int delta  __attribute__((unused))) { screenptr_t s = *this; off++;return s; }
    screenptr_t &operator+=(int delta) { off+=delta; return *this; }
    uint8_t & operator*() {
        CHECK_BOUNDS_SCREEN(off);

        return spectrum_framebuffer.screen[off];

    }
    uint16_t getoff() const { return off.v; }
private:
    u16_8_t off;
};

#endif
