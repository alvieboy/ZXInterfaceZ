#include "label.h"

Label::Label(const char *text,Widget *parent): Widget(parent), m_text(text)
{
    redraw();
}

void Label::drawImpl()
{
    parentDrawImpl();

    screenptr_t screenptr = m_screenptr;
    if (m_text) {
//        ESP_LOGI("WSYS", "TEXT: pointers %04x, xy %d %d", m_screenptr.getoff(), x(), y());
        screenptr.drawstring(m_text);
    }
}

