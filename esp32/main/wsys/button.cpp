#include "button.h"
#include <string.h>
#include "../spectrum_kbd.h"

Button::Button(const char *text, Widget *parent): Widget(parent), m_text(text)
{
    m_accel = KEY_ENTER;
    m_spacing = 0;
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
    for (unsigned i=0;i<width();i++) {
        uint8_t attrval = 0x68;
        if (i<m_spacing)
            attrval = 0x78;
        if (i >= width()-m_spacing)
            attrval = 0x78;

        *attrptr++ = attrval;
    }
}

bool Button::handleEvent(uint8_t type, u16_8_t code)
{
    if (type!=0)
        return false;

    char c = spectrum_kbd__to_ascii(code.v);
    if ((m_accel != 0xff) && (c==m_accel)) {
        m_clicked.emit();
        return true;
    }
    return false;
}


