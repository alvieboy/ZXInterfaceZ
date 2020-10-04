#ifndef __WSYS_WIDGET_H__
#define __WSYS_WIDGET_H__

#include <inttypes.h>
#include <stdlib.h>


#define DAMAGE_ALL      0xFF
#define DAMAGE_WINDOW   0x40
#define DAMAGE_CHILD    0x01
#define DAMAGE_USER1    0x02
#define DAMAGE_USER2    0x04
#define DAMAGE_USER3    0x08

#include "core.h"

class Widget: public WSYSObject
{
public:
    Widget(Widget *parent=NULL);
    virtual ~Widget();

    virtual void draw(bool force=false);
    virtual void setVisible(bool v) { m_visible=v; }
    virtual void resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    virtual void resize(uint8_t w, uint8_t h);
    virtual void move(uint8_t x, uint8_t y);
    void setParent(Widget *w) { m_parent=w; }
    virtual void removeChild(Widget*);
    virtual bool handleEvent(uint8_t type, u16_8_t code);

    uint8_t width() const { return m_w; }
    uint8_t height() const { return m_h; }
    uint8_t x() const { return m_x; }
    uint8_t y() const { return m_y; }

    uint8_t x2() const { return m_x + m_w; }
    uint8_t y2() const { return m_y + m_h; }
    void show() { setVisible(true); }
    void hide() { setVisible(false); }
    bool visible() const { return m_visible; }
    void clear_damage(uint8_t mask);
    void clear_damage();
    uint8_t damage() const { return m_damage; }
    virtual void setdamage(uint8_t mask);
    void redraw() { setdamage(DAMAGE_ALL); }
    static void clearLines(screenptr_t start, unsigned len_chars, unsigned num_lines);
    virtual uint8_t getMinimumWidth() const { return 1; }
    virtual uint8_t getMinimumHeight() const { return 1; }
    virtual void clearChildArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    void parentDrawImpl() {
        clearChildArea(m_x, m_y, m_w, m_h );
    }
    static void setBGLine(attrptr_t attrptr, int width,  uint8_t value);
    void grabKeyboardFocus();
    void releaseKeyboardFocus();

protected:
    virtual void drawImpl() = 0;

    void recalculateScreenPointers();
    screenptr_t m_screenptr;
    attrptr_t m_attrptr;
    Widget *m_parent;
    uint8_t m_x,m_y,m_w,m_h;
    bool m_visible;
    uint8_t m_damage;
};

#endif
