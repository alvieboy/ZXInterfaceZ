#include "button.h"
#include <string.h>
#include "../spectrum_kbd.h"

Button::Button(Widget *parent, const char *text): Widget(parent), m_text(text)
{
    if (text) {
        m_textlen=strlen(text);
        redraw();
    }
}

void Button::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    attrptr_t attrptr = m_attrptr;

    if (m_text) {
        screenptr += (width()/2) - (m_textlen/2);
        screenptr.drawstring(m_text);
    }
    for (int i=0;i<width();i++) {
        *attrptr++ = 0x68;
    }
}

void Button::handleEvent(uint8_t type, u16_8_t code)
{
    if (type!=0)
        return;

    char c = spectrum_kbd__to_ascii(code.v);
    if (c==KEY_ENTER) {
        m_onclick.emit();
    }
}


