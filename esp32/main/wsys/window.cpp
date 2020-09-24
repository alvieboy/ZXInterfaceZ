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
    m_border = 1;
    m_helptext = NULL;
}

void Window::setTitle(const char *title)
{
    m_title = title;
}

void Window::setBorder(uint8_t border)
{
    m_border = border;
}

void Window::setHelpText(const char *text)
{
    m_helptext = text;
    damage(DAMAGE_WINDOW);
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

    setBackground();

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

    if (hasHelpText()) {
        screenptr = m_screenptr;
        screenptr.nextcharline();
        screenptr.nextpixelline();
        screenptr++;
        drawthumbstring(screenptr, m_helptext);
    }

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

    if (hasHelpText()) {
        int i;
        screenptr = m_screenptr;
        screenptr.nextcharline();
        // Improve this.
        for (i=0;i<7;i++) screenptr.nextpixelline();
        screenptr.drawhline(width());
    }
    ESP_LOGI("WSYS","Window redrawn");
}

void Window::resizeEvent()
{
    ESP_LOGI("WSYS","Window resize event");
    if (m_child==NULL)
        return;

    ESP_LOGI("WSYS","Window -> send resize to child %p", m_child);

    unsigned topborder = 1 + m_border;
    if (hasHelpText())
        topborder++;

    m_child->resize(m_x+1, m_y+topborder, m_w-2, m_h-(m_border+topborder));
}

void Window::setBGLine(attrptr_t attrptr, uint8_t value)
{
    for (int i=0;i<m_w;i++) {
            *attrptr++ = value;
    }
}

void Window::setBackground()
{
    int c = (m_h) * 8;
    screenptr_t screenptr = m_screenptr;
    attrptr_t attrptr = m_attrptr;
    screenptr_t save;
    attrptr_t attrptr2;
    const uint8_t normal_bg = 0x78;
    const uint8_t help_bg   = 0x38;



    while (c--) {
        save = screenptr;
        for (int i=0;i<m_w;i++) {
            *screenptr++ = 0x0;
        }
        screenptr = save;
        screenptr.nextpixelline();
    }

    c = (m_h)-1;

    // Header
    setBGLine(attrptr, normal_bg);
    attrptr.nextline();
    if (hasHelpText()) {
        c--;
        setBGLine(attrptr, help_bg);
        attrptr.nextline();

    }

    while (c--) {
        setBGLine(attrptr, normal_bg);
        attrptr.nextline();
    }
}

Window::~Window()
{
    screen__removeWindow(this);
}


bool Window::needRedraw() {
    if (m_child) {
        if (m_child->damage())
            return true;
    }
    if (damage()&DAMAGE_WINDOW)
        return true;
    return false;
}
