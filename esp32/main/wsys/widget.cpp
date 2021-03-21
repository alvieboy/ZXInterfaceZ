#include "widget.h"
#include "screen.h"
#include <cstring>

extern "C" {
#include "esp_log.h"
};

void Widget::resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    recalculateScreenPointers();
}

void Widget::resize(uint8_t w, uint8_t h)
{
    m_w = w;
    m_h = h;
    recalculateScreenPointers();
}

void Widget::move(uint8_t x, uint8_t y)
{
    m_x = x;
    m_y = y;
    recalculateScreenPointers();
}

void Widget::draw(bool force)
{
    if (isVisible()) {
        drawImpl();
    } else {
        WSYS_LOGI("Not redrawing %s, not visible", CLASSNAME(*this));
    }
}

void Widget::recalculateScreenPointers()
{
    m_screenptr.fromxy(m_x, m_y);
    m_attrptr.fromxy(m_x, m_y);
#if 0
    WSYS_LOGI( "%p Recomputing xy %d %d", this, m_x, m_y);
    WSYS_LOGI( "%p Recomputed pointers %04x %04x", this, m_screenptr.getoff(), m_attrptr.getoff() );
#endif
}

bool Widget::handleEvent(wsys_input_event_t evt)
{
    return false;
}

Widget::~Widget()
{
    screen__releaseKeyboardFocus(this);
}

void Widget::removeChild(Widget*)
{

}

Widget::Widget(Widget *parent): m_parent(parent)
{
    m_x = 0;
    m_y = 0;
    m_w = 0;
    m_h = 0;
    m_minx = 1;
    m_miny = 1;
    m_canfocus = false;
    m_hasfocus = false;
    m_focusnext = this;
    m_focusprev = this;
    m_visible = true;
    redraw();
}

void Widget::setdamage(uint8_t mask)
{
    m_damage |= mask;
    //WSYS_LOGI( "damage %d parent %p\n", damage(), m_parent);

    Widget *parent = m_parent;
    while (parent) {
        parent->setdamage(DAMAGE_CHILD);
        parent = parent->m_parent;
    }
}


void Widget::clear_damage(uint8_t mask)
{
    m_damage &= ~mask;
}

void Widget::clear_damage()
{
    m_damage = 0;
}

void Widget::clearLines(screenptr_t start, unsigned len_chars, unsigned num_lines)
{
    int c = num_lines * 8;
    while (c--) {
        screenptr_t ptr = start;
        for (int i=0;i<len_chars;i++) {
            *ptr++ = 0x0;
        }
        start.nextpixelline();
    }
}

void Widget::clearChildArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (m_parent) {
        //WSYS_LOGI(" -> parent %p clearChildArea()", m_parent);
        m_parent->clearChildArea(x,y,w,h);
    } else {
      //  WSYS_LOGI(" -> no parent");
    }
}

void Widget::setBGLine(attrptr_t attrptr, int len, uint8_t value)
{
    for (int i=0;i<len;i++) {
        *attrptr++ = value;
    }
}

void Widget::setVisible(bool v)
{
    bool needredraw = false;
    WSYS_LOGI("Setting visibility %s to %d", CLASSNAME(*this), v);
    if (!m_visible && v) {
        needredraw=true;
    }
    m_visible=v;
    if (needredraw)
        redraw();
    visibilityOrFocusPolicyChanged(this);
}

void Widget::grabKeyboardFocus()
{
    screen__grabKeyboardFocus(this);
}

void Widget::releaseKeyboardFocus()
{
    screen__releaseKeyboardFocus(this);
}

void Widget::setFocus(bool focus)
{
    if (!canFocus()) {
        WSYS_LOGI("NOT focusing %s\n",CLASSNAME(*this));
        return;
    }

    WSYS_LOGI("setFocus %d %s\n",focus, CLASSNAME(*this));
    m_hasfocus=focus;

    if (focus)
        focusIn();
    else
        focusOut();
}

void Widget::focusInsertAfter(Widget *Z)
{
    Widget *C = this;
    Widget *C_prev = C->m_focusprev;

#ifdef DEBUG_FOCUS
    WSYS_LOGI("Dumping focus tree before insert in %p (%s)", Z, CLASSNAME(*Z));
    Z->dumpFocus();

    WSYS_LOGI("  - Step 1:");
    C->dumpFocus();
    Z->dumpFocus();
#endif
    C->m_focusprev->m_focusnext = Z->m_focusnext;


#ifdef DEBUG_FOCUS
    WSYS_LOGI("  - Step 2:");
    C->dumpFocus();
    Z->dumpFocus();
#endif
    C->m_focusprev = Z;

#ifdef DEBUG_FOCUS
    WSYS_LOGI("  - Step 3:");
    C->dumpFocus();
    Z->dumpFocus();
#endif

    Z->m_focusnext->m_focusprev = C_prev;

#ifdef DEBUG_FOCUS

    WSYS_LOGI("  - Step 4:");
    C->dumpFocus();
    Z->dumpFocus();
#endif

    Z->m_focusnext = C;   /* Confirm */

#ifdef DEBUG_FOCUS

    WSYS_LOGI("Dumping focus tree after insert in %p (%s)", Z, CLASSNAME(*Z));
    dumpFocusTree(Z);
#endif
}


void Widget::dumpFocus()
{
    WSYS_LOGI("-- Focus for item %s %p", CLASSNAME(*this), this);
    WSYS_LOGI("  > Next points to %s %p", m_focusnext==this?"ITSELF":CLASSNAME(*m_focusnext), this);
    WSYS_LOGI("  > Prev points to %s %p", m_focusprev==this?"ITSELF":CLASSNAME(*m_focusprev), this);
}

void Widget::dumpFocusTree(Widget *start)
{
    Widget *s;
    WSYS_LOGI("Focus tree: ");
    s=start;
    do {
        char flags[64];
        flags[0] = '\0';
        if (s->canFocus()) {
            strcat(flags," FOCUSABLE");
        }
        if (!s->isVisible()) {
            strcat(flags," HIDDEN");
        }

        WSYS_LOGI(" > %s %p [%s ]", CLASSNAME(*s), s, flags);
        s = s->m_focusnext;

    } while (s!=start);
}


bool Widget::isVisible() const
{
    if (!m_visible)
        return m_visible;
    if (m_parent) {
        bool v = m_parent->isVisible();
        WSYS_LOGI("Parent %s (%p) visible %d", CLASSNAME(*m_parent), m_parent, v);
        return v;
    }
    return m_visible;
}
