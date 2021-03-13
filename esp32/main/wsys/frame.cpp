#include "frame.h"
#include <stdlib.h>
#include "charmap.h"
#include <string.h>

Frame::Frame(const char *title, uint8_t w, uint8_t h, bool drawbackground): Bin(NULL)
{
    m_w = w;
    m_h = h;
    m_border = 1;
    m_drawbackground = drawbackground;
    setTitle(title);
}

void Frame::setTitle(const char *title)
{
    strncpy(m_title,title,sizeof(m_title));
    redraw();
}

void Frame::setBorder(uint8_t border)
{
    m_border = border;
}

void Frame::drawFrame()
{
    screenptr_t screenptr = m_screenptr;
    screenptr_t saveptr;

    //setBackground();

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

    screenptr = m_screenptr;

    //push_charset(COMPUTER_CHARSET);
    //screenptr.drawstring( m_title );
    //pop_charset();
    drawthumbstring(screenptr, m_title,2);

    screenptr = m_screenptr;
    screenptr.nextpixelline(7);
    screenptr.drawhline(m_w);

}

void Frame::resizeEvent()
{
    WSYS_LOGI("Frame resize event");
    if (m_child==NULL)
        return;

    WSYS_LOGI("Frame -> send resize to child %p", m_child);

    unsigned topborder = 1 + m_border;

    m_child->resize(m_x+1, m_y+topborder, m_w-2, m_h-(m_border+topborder));
}

void Frame::drawImpl()
{
    parentDrawImpl();
    if (damage() & ~DAMAGE_CHILD) {
        drawFrame();
    }
}
#if 0
void Frame::setBackground()
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
#endif

Frame::~Frame()
{
}


bool Frame::needRedraw() {
    if (m_child) {
        if (m_child->damage())
            return true;
    }
    return false;
}

void Frame::clearChildArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
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
