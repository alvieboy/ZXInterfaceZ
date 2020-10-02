#include "window.h"
#include <stdlib.h>
#include "charmap.h"
#include "screen.h"
extern "C" {
#include "esp_log.h"
};

#define DAMAGE_HELPTEXT DAMAGE_USER1

Window::Window(const char *title, uint8_t w, uint8_t h): Bin(NULL)
{
    m_w = w;
    m_h = h;
    m_title = title;
    m_border = 1;
    m_helptext = NULL;
    m_statuslines = 0;
    m_statustext = NULL;
}

void Window::setTitle(const char *title)
{
    m_title = title;
}

void Window::setBorder(uint8_t border)
{
    m_border = border;
}

void Window::setStatusLines(uint8_t lines)
{
    m_statuslines = lines;
}

void Window::setWindowHelpText(const char *text)
{
    m_helptext = text;
    setdamage(DAMAGE_WINDOW);
}

void Window::displayHelpText(const char *c)
{
    //void maxCharsForPixelCount(const char *offset, unsigned pixels);
    WSYS_LOGI("SHOW HELP: %s", c);
    m_statustext = c;
    setdamage(DAMAGE_HELPTEXT);
}

void Window::fillHeaderLine(attrptr_t attr)
{
    uint8_t w = m_w - 1;
    while (w--) {
        *attr++ = 0x07;
    }
    *attr = 0x00; // last one black
}

void Window::drawStatus()
{
    screenptr_t screenptr;
    if (m_statuslines>0) {
        screenptr.fromxy(x(), y() + height() - m_statuslines );
        //screenptr.drawhline(width());
        unsigned count = 8 * m_statuslines - 1;
        WSYS_LOGI( "Redrawing status, count %d", count);
        screenptr.nextpixelline();
        while (count--) {
            screenptr_t temp = screenptr;
            temp++;
            for (int i=0;i<m_w-2;i++) {
                *temp++ = 0x00;
            }
            screenptr.nextpixelline();
        }
        if (m_statustext) {
            screenptr.fromxy(x()+1, y() + height() - m_statuslines );
            screenptr.nextpixelline();
            screenptr.nextpixelline();
            drawthumbstring(screenptr, m_statustext);

        }
    }

}

void Window::drawWindowCore()
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
    screenptr = screenptr.drawchar(HH);
    screenptr = screenptr.drawchar(HH);
    screenptr = screenptr.drawchar(HH);
    screenptr = screenptr.drawchar(HH);
    screenptr = screenptr.drawchar(HH);

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

    if (m_statuslines>0) {
        screenptr.fromxy(x(), y() + height() - m_statuslines);
        screenptr.drawhline(width());
    }

    WSYS_LOGI("Window redrawn");
}

void Window::resizeEvent()
{
    WSYS_LOGI("Window resize event");
    if (m_child==NULL)
        return;

    WSYS_LOGI("Window -> send resize to child %p", m_child);

    unsigned topborder = 1 + m_border;

    if (hasHelpText())
        topborder++;

    m_child->resize(m_x+1, m_y+topborder, m_w-2, m_h-(m_border+topborder+m_statuslines));
}

void Window::drawImpl()
{
    if (damage() & ~DAMAGE_CHILD) {
        if (damage() & DAMAGE_WINDOW)
            drawWindowCore();
        if (damage() & DAMAGE_HELPTEXT) {
            drawStatus();
        }
    }
}

void Window::setBGLine(attrptr_t attrptr, int len, uint8_t value)
{
    for (int i=0;i<len;i++) {
        *attrptr++ = value;
    }
}

void Window::setBackground()
{
    int c = (m_h) * 8;
    screenptr_t screenptr = m_screenptr;
    attrptr_t attrptr = m_attrptr;
    screenptr_t save;
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
    setBGLine(attrptr, m_w, normal_bg);
    attrptr.nextline();
    if (hasHelpText()) {
        c--;
        setBGLine(attrptr, m_w, help_bg);
        attrptr.nextline();

    }

    while (c>m_statuslines) {
        setBGLine(attrptr, m_w, normal_bg);
        attrptr.nextline();
        c--;
    }

    for (c=0;c<m_statuslines;c++) {
        setBGLine(attrptr, m_w, help_bg);
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

void Window::draw(bool force)
{
    if (force)
        setdamage(DAMAGE_WINDOW|DAMAGE_HELPTEXT);
    Bin::draw(force);
}


void Window::clearChildArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    screenptr_t r;
    attrptr_t a;
    r.fromxy(x,y);
    a.fromxy(x,y);
    WSYS_LOGI("clear xy %d %d\n",x,y);
    clearLines( r, w, h);

    for (int c=0; c<h;c++) {
        setBGLine(a, w, 0x78);
        a.nextline();
    }

}
