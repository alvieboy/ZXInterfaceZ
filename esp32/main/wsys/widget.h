#ifndef __WSYS_WIDGET_H__
#define __WSYS_WIDGET_H__

#include <inttypes.h>
#include <stdlib.h>

#include "core.h"

class Widget
{
public:
    Widget(Widget *parent=NULL);
    virtual ~Widget();

    virtual void draw();
    virtual void setVisible(bool v) { m_visible=v; if (v) draw(); }
    virtual void resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    virtual void resize(uint8_t w, uint8_t h);
    virtual void move(uint8_t x, uint8_t y);
    void setParent(Widget *w) { m_parent=w; }
    virtual void removeChild(Widget*);
    virtual void handleEvent(uint8_t type, u16_8_t code);

    uint8_t width() const { return m_w; }
    uint8_t height() const { return m_h; }
    uint8_t x() const { return m_x; }
    uint8_t y() const { return m_y; }

    uint8_t x2() const { return m_x + m_w; }
    uint8_t y2() const { return m_y + m_h; }

protected:
    virtual void drawImpl() = 0;

    void recalculateScreenPointers();
    screenptr_t m_screenptr;
    attrptr_t m_attrptr;
    Widget *m_parent;
    uint8_t m_x,m_y,m_w,m_h;
    bool m_visible;
};

#endif
