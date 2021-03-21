#include "editbox.h"
#include "color.h"
#include "spectrum_kbd.h"
#include "charmap.h"

EditBox::EditBox(const char *text, Widget *parent): Widget(parent)
{
    //m_editable = false;
    if (text)
        m_text = text;
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

    WSYS_LOGI("EditBox: has focus %d policy %d\n", hasFocus(), canFocus());

    if (hasFocus()) {
        setBGLine(m_attrptr, m_w, MAKECOLORA(BLACK, BLUE|GREEN, BRIGHT));
        *attrptr = MAKECOLORA(BLACK, WHITE, BRIGHT|FLASH);
        screenptr.drawascii('L');
    } else {
        setBGLine(m_attrptr, m_w, MAKECOLOR(WHITE, BLUE));
    }
}

void EditBox::focusIn()
{
    WSYS_LOGI("Focus in");
    redraw();
}

void EditBox::focusOut()
{
    WSYS_LOGI("Focus out");
    redraw();
}

bool EditBox::handleEvent(wsys_input_event_t evt)
{
    int ret = false;
    if (evt.type!=WSYS_INPUT_EVENT_KBD)
        return ret;

    unsigned char c = spectrum_kbd__to_ascii(evt.code.v);

    switch (c) {
    case KEY_BACKSPACE:
        if (m_text.size()>0) {
            m_text.pop_back();
            redraw();
        }
        ret=true;
        break;
    case KEY_ENTER:
        m_enter.emit();
        ret=true;
        break;
    default:
        if (IS_PRINTABLE(c)) {
            if (m_text.size() < (unsigned)width()-1) {
                m_text+=c;
                redraw();
            }
            ret=true;
        }
        break;
    }

    return ret;
}


void EditBox::setEditable(bool e)
{
    setFocusPolicy(e);
    /*if (m_editable!=e) {
        if (e)
            grabKeyboardFocus();
        else
            releaseKeyboardFocus();
    }
    m_editable=e;
    */
    redraw();
};
