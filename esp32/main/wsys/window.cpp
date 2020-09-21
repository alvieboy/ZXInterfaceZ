#include "window.h"
#include <stdlib.h>
#include "charmap.h"
#include "screen.h"
extern "C" {
#include "esp_log.h"
};

Window::Window(const char *title, uint8_t w, uint8_t h): Bin(NULL)
{
    m_w = w;
    m_h = h;
    m_title = title;
}

void Window::setTitle(const char *title)
{
    m_title = title;
}

void Window::fillHeaderLine(attrptr_t attr)
{
    uint8_t w = m_w - 1;
    while (w--) {
        *attr++ = 0x07;
    }
    *attr = 0x00; // last one black
}

void Window::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    attrptr_t attrptr = m_attrptr;
    screenptr_t saveptr;

    setBackground(0x78);

    screenptr.nextcharline();
    int i;
    for (i=0;i<m_h-1;i++) {
        saveptr = screenptr;
        screenptr.drawchar( LEFTVERTICAL );
        screenptr+=m_w-1;
        screenptr.drawchar( RIGHTVERTICAL );
        screenptr = saveptr;
        screenptr.nextcharline();
    }

    screenptr.drawhline(m_w);
    fillHeaderLine(m_attrptr);

    screenptr = m_screenptr;
    screenptr++;

    screenptr.drawstring( m_title );

    screenptr = m_screenptr;
    screenptr += m_w - 6;
    screenptr.drawchar(HH)++;
    screenptr.drawchar(HH)++;
    screenptr.drawchar(HH)++;
    screenptr.drawchar(HH)++;
    screenptr.drawchar(HH)++;

    attrptr = m_attrptr;
    attrptr += m_w - 6;
    *attrptr++ = 0x42;
    *attrptr++ = 0x56;
    *attrptr++ = 0x74;
    *attrptr++ = 0x65;
    *attrptr++ = 0x68;
    *attrptr++ = 0x00;

    ESP_LOGI("WSYS","Window redrawn");
}

void Window::resizeEvent()
{
    ESP_LOGI("WSYS","Window resize event");
    if (m_child==NULL)
        return;

    ESP_LOGI("WSYS","Window -> send resize to child %p", m_child);

    m_child->resize(m_x+1, m_y+2, m_w-2, m_h-3);
}

void Window::setBackground(uint8_t value)
{
    int c = (m_h) * 8;
    screenptr_t screenptr = m_screenptr;
    attrptr_t attrptr = m_attrptr;
    screenptr_t save;
    attrptr_t savea;

    while (c--) {
        save = screenptr;
        for (int i=0;i<m_w;i++) {
            *screenptr++ = 0x0;
        }
        screenptr = save;
        screenptr.nextpixelline();
    }

    c = (m_h);

    while (c--) {
        savea = attrptr;
        for (int i=0;i<m_w;i++) {
            *attrptr++ = value;
        }
        attrptr = savea;
        attrptr.nextline();
    }
}

Window::~Window()
{
    screen__removeWindow(this);
}


