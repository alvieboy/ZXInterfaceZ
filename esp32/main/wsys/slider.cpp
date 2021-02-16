#include "slider.h"
#include "spectrum_kbd.h"


static const uint8_t lookup6[7] = {
    0x80,
    0xA0,
    0xB0,
    0xB8,
    0xBC,
    0xBE,
    0xBF
};
static const uint8_t lookupf6[7] = {
    0x01,
    0x81,
    0xC1,
    0xE1,
    0xF1,
    0xF9,
    0xFD
};

void SliderBase::drawSliderLine(screenptr_t &ptr, unsigned w, unsigned vlen)
{
    screenptr_t ptr2 = ptr;
    uint8_t mask = 0;

    unsigned dlen = vlen>6?6:vlen;
    mask = lookup6[dlen];
    vlen-=dlen;

    ptr2.drawvalue(mask);
    ptr2++;
    w-=2;
    while (w--) {
        if (vlen>0) {
            dlen = vlen>8?8:vlen;
            mask = (0xFF00 >> dlen);
            vlen-=dlen;
            ptr2.drawvalue(mask);
        } else {
            ptr2.drawvalue(0x0);
        }
        ptr2++;
    }
    dlen = vlen>6?6:vlen;
    mask = lookupf6[dlen];
    vlen-=dlen;

    ptr2.drawvalue(mask);
    ptr.nextpixelline();
}

void SliderBase::drawSlider(float percentage, const char *text)
{
    unsigned w = width();
    WSYS_LOGI("Width %d, percent %f",w,percentage);
    w-=2; // Left accel + spacing
    w-=2; // Right accel + spacing
    screenptr_t ptr = m_screenptr;
    screenptr_t ptr2;
    // Draw accel chars
    ptr = ptr.drawascii(m_accel_decrease);
    ptr++;

    ptr2 = ptr.drawhline(w);
    ptr2++;
    ptr2.drawascii(m_accel_increase);

    ptr.nextpixelline();

    unsigned h = (height()*8) - 4;

    // Compute slider width.
    float slider_width = (w*8)-2;
    slider_width *= percentage;
    unsigned slider_width_u = (unsigned)slider_width;

    drawSliderLine(ptr, w, 0);
    for (unsigned j = 0; j<h; j++) {
        drawSliderLine(ptr, w, slider_width);
    }
    drawSliderLine(ptr, w, 0);

    ptr.drawhline(w);
    // Draw text.
    ptr = m_screenptr;
    ptr.nextpixelline();
    ptr.nextpixelline();
    ptr += width()/2 - 4;
    //drawthumbstringxor( ptr, "10%");
}

bool SliderBase::handleEvent(uint8_t type, u16_8_t code)
{
    bool handled = false;
    if (type!=0)
        return handled;

    unsigned char c = spectrum_kbd__to_ascii(code.v);
    do {
        if (c==KEY_UNKNOWN)
            break;
        if (m_accel_increase==c) {
            handled=true;
            inc(false);
            break;
        }
        if (m_accel_increase_h==c) {
            handled=true;
            inc(true);
            break;
        }
        if (m_accel_decrease==c) {
            handled=true;
            dec(false);
            break;
        }
        if (m_accel_decrease_h==c) {
            handled=true;
            dec(true);
            break;
        }
    } while (0);
    return handled;
}

SliderBase::SliderBase()
{
    setFocusPolicy(true);
    m_accel_decrease = KEY_UNKNOWN;
    m_accel_decrease_h = KEY_UNKNOWN;
    m_accel_increase = KEY_UNKNOWN;
    m_accel_increase_h = KEY_UNKNOWN;
}
