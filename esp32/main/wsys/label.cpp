#include "label.h"

Label::Label(const char *text,Widget *parent): Widget(parent), m_text(text)
{
    m_background = -1;
    m_spacing = 0;
    redraw();
}

void Label::drawImpl()
{
    parentDrawImpl();

    screenptr_t screenptr = m_screenptr;
    if (m_text.size()) {
        screenptr += m_spacing;
        screenptr.drawstring(m_text.c_str());
    }

    if (m_background>=0) {
        setBGLine(m_attrptr, m_w, m_background & 0xff);
    }

}

