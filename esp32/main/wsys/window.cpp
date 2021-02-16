#include "window.h"
#include <stdlib.h>
#include "charmap.h"
#include "screen.h"
extern "C" {
#include "esp_log.h"
};
#include "spectrum_kbd.h"

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
    m_focusWidget = NULL;
    m_focusnextkey = KEY_RIGHT;
    m_focusprevkey = KEY_LEFT;
}

void Window::setFocusKeys(uint8_t next, uint8_t prev)
{
    m_focusnextkey = next;
    m_focusprevkey = prev;
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
        *attr++ = attr_t(WHITE);//BLACK, CYAN, true);//WHITE);
    }
    *attr = attr_t(BLACK);
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

    push_charset(COMPUTER_CHARSET);
    screenptr.drawstring( m_title );
    pop_charset();

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
    *attrptr++ = attr_t(RED, BLACK, true);//0x42;
    *attrptr++ = attr_t(YELLOW, RED, true);//0x56;
    *attrptr++ = attr_t(GREEN, YELLOW, true);//0x56;0x74;
    *attrptr++ = attr_t(CYAN, GREEN, true);//0x56;0x74;0x65;
    *attrptr++ = attr_t(BLACK, CYAN, true);//0x56;0x74;0x65;0x68;
    *attrptr++ = attr_t(BLACK);//0x00;

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
    WSYS_LOGI("clear xy %d %d, width=%d, height=%d\n",x,y,w,h);
    clearLines( r, w, h);

    for (int c=0; c<h;c++) {
        setBGLine(a, w, 0x78);
        a.nextline();
    }

}

void Window::dumpFocusTree()
{
    Widget *start = m_focusWidget;
    WSYS_LOGI("Focus tree: ");
    if (!start) {
        start = this;
        WSYS_LOGI("No focus widget! child=%p, start at %p\n", m_child, this);
    }
    Widget::dumpFocusTree(start);
}

void Window::focusNextPrev(bool next)
{
    Widget *start = m_focusWidget;
    Widget *candidate;

    dumpFocusTree();

    if (!start)
        start = this;

    candidate = start;

    do {

        if (next) {
            candidate = candidate->m_focusnext;
        } else {
            candidate = candidate->m_focusprev;
        }

        WSYS_LOGI("next=%d focus=%p this=%p candidate=%p start=%p", next, m_focusWidget, this, candidate, start);

        if (candidate==start) {
            WSYS_LOGI("Same as start");
            break;
        }

        if (candidate!=m_focusWidget) {
            if (candidate->canFocus() && candidate->isVisible()) {
                WSYS_LOGI("is focusable %s", CLASSNAME(*candidate));
                if (m_focusWidget) {
                    m_focusWidget->setFocus(false);
                }
                m_focusWidget = candidate;
                m_focusWidget->setFocus(true);
                break;
            }
        }

    } while (candidate!=start);
    WSYS_LOGI("Focus finished");
}


void Window::setChild(Widget *c)
{
    Bin::setChild(c);
    if (isVisible()) {
        focusNextPrev(true);
    }
}

void Window::setVisible(bool visible)
{
    bool changed = false;
    WSYS_LOGI("Visibility changed %d\n", visible);
    if (visible!=m_visible)
        changed = true;

    Bin::setVisible(visible);

    if (isVisible() && !m_focusWidget) {
        WSYS_LOGI("Focusing changed");
        focusNextPrev(true);
    }
    screen__windowVisibilityChanged(this, visible);
}



bool Window::handleEvent(uint8_t type, u16_8_t code)
{
    bool handled = false;
    if (m_focusWidget && m_focusWidget!=this) {
        WSYS_LOGI("Dispatch event %s\n", CLASSNAME(*m_focusWidget));
        handled = m_focusWidget->handleEvent(type, code);
    }
    if (handled)
        return handled;

    if (type==0) {
        unsigned char c = spectrum_kbd__to_ascii(code.v);

        if (c==m_focusnextkey) {
            WSYS_LOGI("request focus next");
            handled = true;
            focusNextPrev(true);
        } else if (c==m_focusprevkey) {
            WSYS_LOGI("request focus prev");
            handled = true;
            focusNextPrev(false);
        }
    }
    return handled;
}

void Window::visibilityOrFocusChanged(Widget *w)
{
    if (w==m_focusWidget) {
        focusNextPrev(true);
    }
}
