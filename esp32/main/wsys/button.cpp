#include "button.h"
#include <string.h>
#include "../spectrum_kbd.h"
#include "color.h"

Button::Button(const char *text, Widget *parent): Widget(parent), m_text(text)
{
    setFocusPolicy(true);
    m_thumb = false;
    m_accel = KEY_ENTER;
    m_spacing = 0;
    if (text) {
        m_textlen=strlen(text);
        redraw();
    }
}

void Button::setThumbFont(bool yes)
{
    m_thumb = yes;
    redraw();
}

void Button::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    attrptr_t attrptr = m_attrptr;
    WSYS_LOGI("Button redraw");

    if (m_text) {
        if (m_thumb) {
            screenptr.nextpixelline();
            int deltapix = (width()*4) - (m_textlen*5/2);
            screenptr += (deltapix/8);
            deltapix &=7; // Get remaining

            drawthumbstring(screenptr, m_text, deltapix);

        } else {
            screenptr += (width()/2) - (m_textlen/2);
            screenptr.drawstring(m_text);
        }
    }
    for (unsigned i=0;i<width();i++) {
        uint8_t attrval;
        if (!hasFocus()) {
            attrval = MAKECOLOR(BLACK, WHITE);
        } else {
            attrval = 0x68;
            if (i<m_spacing)
                attrval = 0x78;
            if (i >= (unsigned)width()-m_spacing)
                attrval = 0x78;
        }
        *attrptr++ = attrval;
    }
}

bool Button::handleEvent(wsys_input_event_t evt)
{
    bool ret = false;

    if (evt.type ==WSYS_INPUT_EVENT_KBD) {

        unsigned char c = spectrum_kbd__to_ascii(evt.code.v);
        // HACK: makes no sense not to use ENTER here.
        if ((m_accel >=0) && (c==m_accel)) {
            m_clicked.emit();
            ret = true;
        }
    } else if (evt.type == WSYS_INPUT_EVENT_JOYSTICK && evt.joy_on) {
        if (evt.joy_action==JOY_FIRE1) {
            m_clicked.emit();
            ret = true;
        }
    }
    return ret;
}


void Button::focusIn()
{
    redraw();
}
void Button::focusOut()
{
    redraw();
}
