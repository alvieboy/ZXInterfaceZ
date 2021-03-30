#ifndef __WSYS_WIDGET_H__
#define __WSYS_WIDGET_H__

#include <inttypes.h>
#include <stdlib.h>
#include "screen.h"
#include "minmax.h"

#define DAMAGE_ALL      0xFF
#define DAMAGE_WINDOW   0x40
#define DAMAGE_CHILD    0x01
#define DAMAGE_USER1    0x02
#define DAMAGE_USER2    0x04
#define DAMAGE_USER3    0x08

#include "core.h"
#include <cassert>
/**
 * \ingroup wsyswidget
 * \brief Base class for all widget types
 */
class Widget: public WSYSObject
{
public:
    Widget(Widget *parent=NULL);
    virtual ~Widget();

    virtual void draw(bool force=false);
    virtual void setVisible(bool v);
    virtual void resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    virtual void resize(uint8_t w, uint8_t h);
    virtual void move(uint8_t x, uint8_t y);
    void setParent(Widget *w) {
        assert(m_parent==NULL);
        m_parent=w;
    }
    virtual void removeChild(Widget*);
    virtual bool handleEvent(wsys_input_event_t);

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
    virtual void clearChildArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    void parentDrawImpl() {
        clearChildArea(m_x, m_y, m_w, m_h );
    }
    static void setBGLine(attrptr_t attrptr, int width,  uint8_t value);
    void grabKeyboardFocus();
    void releaseKeyboardFocus();
    void setFocusPolicy(bool focus) { m_canfocus=focus; visibilityOrFocusPolicyChanged(this); }
    virtual bool canFocus() const { return m_canfocus; }
    virtual void focusIn() {};
    virtual void focusOut() {};
    bool hasFocus() const { return m_hasfocus; }

    virtual void setFocus(bool focus);
    virtual uint8_t getMinimumWidth() const { return MAX(m_minx,implMinimumWidth()); }
    virtual uint8_t getMinimumHeight() const { return MAX(m_miny,implMinimumHeight()); }
    virtual bool isVisible() const;
    virtual void visibilityOrFocusPolicyChanged(Widget *w) {
        if (m_parent)
            m_parent->visibilityOrFocusPolicyChanged(w);
    }
    void focusInsertAfter(Widget *where);

    static void dumpFocusTree(Widget *);
    void dumpFocus();
private:
    virtual uint8_t implMinimumWidth() const { return 1; }
    virtual uint8_t implMinimumHeight() const { return 1; }

protected:
    virtual void drawImpl() = 0;
    void recalculateScreenPointers();

    virtual void focusNextPrev(bool next) {
        if (m_parent)
            m_parent->focusNextPrev(next);
    }

protected:
    friend class Window;
    Widget *m_focusnext;
    Widget *m_focusprev;
protected:
    screenptr_t m_screenptr;
    attrptr_t m_attrptr;
    Widget *m_parent;
    uint8_t m_x,m_y,m_w,m_h;
    int8_t m_minx, m_miny;
    uint8_t m_damage;
    bool m_canfocus;
    bool m_hasfocus;
    bool m_visible;
};

#endif
