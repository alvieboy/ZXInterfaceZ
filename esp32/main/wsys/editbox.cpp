#include "editbox.h"
#include "color.h"
#include "spectrum_kbd.h"
#include "charmap.h"

EditBox::EditBox(const char *text, Widget *parent): Widget(parent), m_text(text)
{
    m_editable = false;
    redraw();
}

void EditBox::drawImpl()
{
    parentDrawImpl();

    screenptr_t screenptr = m_screenptr;
    attrptr_t attrptr = m_attrptr;

    unsigned len = m_text.size();

    if (len) {
        screenptr = screenptr.drawstring(m_text.c_str());
    }

    attrptr+=len;

    if (m_editable) {
        setBGLine(m_attrptr, m_w, MAKECOLORA(BLACK, BLUE|GREEN, BRIGHT));
        *attrptr = MAKECOLORA(BLACK, WHITE, BRIGHT|FLASH);
        screenptr.drawascii('L');
    } else {
        setBGLine(m_attrptr, m_w, MAKECOLOR(WHITE, BLUE));
    }
}



bool EditBox::handleEvent(uint8_t type, u16_8_t code)
{
    if (!m_editable || (type!=0))
        return false;

    char c = spectrum_kbd__to_ascii(code.v);

    switch (c) {
    case KEY_BACKSPACE:
        if (m_text.size()>0) {
            m_text.pop_back();
            redraw();
        }
        break;
    case KEY_ENTER:
        m_enter.emit();
        break;
    default:
        if (IS_PRINTABLE(c)) {
            if (m_text.size() < width()-1) {
                m_text+=c;
                redraw();
            }
        }
        break;
    }

    return true;
}


void EditBox::setEditable(bool e)
{
    if (m_editable!=e) {
        if (e)
            grabKeyboardFocus();
        else
            releaseKeyboardFocus();
    }
    m_editable=e;
    redraw();
};
