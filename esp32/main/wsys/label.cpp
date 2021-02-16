#include "label.h"
#include <cstring>

static int count_newlines(const char *c)
{
    int newline = 0;
    while (*c) {
        if (*c=='\n')
            newline++;
        c++;
    }
    return newline;
}

Label::Label(const char *text,Widget *parent): Widget(parent), m_text(text)
{
    m_background = -1;
    m_spacing = 0;
    m_newlines = count_newlines(text);
    redraw();
}

void Label::setText(const char *text) {
    m_text=text;
    redraw();
}


uint8_t Label::getMinimumHeight() const {
    return m_newlines+1;
}

uint8_t Label::getMinimumWidth() const
{
    uint8_t size = 0;
    uint8_t linesize = 0;
    const char *start = m_text.c_str();
    do {
        const char *nl = strchr(start,'\n');
        if (nl) {
            linesize = nl-start;
            if (size<linesize)
                size=linesize;
        } else {
            linesize = strlen(start);
            if (size<linesize)
                size=linesize;
            break;
        }
    } while (1);
    return size + 2*m_spacing;
}

void Label::drawLines(screenptr_t ptr)
{
    const char *start = m_text.c_str();
    do {
        const char *nl = strchr(start,'\n');
        if (nl) {
            ptr.drawstringn(start, nl-start);
            start = nl+1;
            ptr.nextcharline();
        } else {
            ptr.drawstring(start);
            break;
        }
    } while (1);
}

void Label::drawImpl()
{
    parentDrawImpl();

    screenptr_t screenptr = m_screenptr;
    if (m_text.size()) {
        screenptr += m_spacing;
        drawLines(screenptr);
    }

    if (m_background>=0) {
        setBGLine(m_attrptr, m_w, m_background & 0xff);
    }

}

