#include "label.h"

Label::Label(Widget *parent, const char *text): Widget(parent), m_text(text)
{
}

void Label::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    if (m_text) {
//        ESP_LOGI("WSYS", "TEXT: pointers %04x, xy %d %d", m_screenptr.getoff(), x(), y());
        screenptr.drawstring(m_text);
    }
}

