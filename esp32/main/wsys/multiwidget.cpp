#include "multiwidget.h"
#include "spectrum_kbd.h"
//#define MAX_CHILDS 4

void MultiWidget::draw(bool force)
{
    WSYS_LOGI( "MultiWidget::draw force=%d damage=0x%02x", force?1:0, damage());

    if (force || (damage() & ~DAMAGE_CHILD)) { // If any bits beside child, then draw
        drawImpl();
    }

    for (int i=0;i<m_numchilds;i++) {
        WSYS_LOGI( "MultiWidget::draw child force=%d child_damage=0x%02x", force?1:0, m_childs[i]->damage());

        if ((force || (damage() & DAMAGE_CHILD)))  {
            if (force || m_childs[i]->damage())
                m_childs[i]->draw(force);

            m_childs[i]->clear_damage();
        }
    }

    clear_damage();


}

bool MultiWidget::handleLocalEvent(uint8_t type, u16_8_t code)
{
    WSYS_LOGI("MultiWidget handleLocalEvent");
    if (type==0) {
        unsigned char c = spectrum_kbd__to_ascii(code.v);
        WSYS_LOGI("MultiWidget handleLocalEvent key %02x", c);

        if (c==KEY_RIGHT) {
            focusNext();
        }
    }
    return false;
}

bool MultiWidget::handleEvent(uint8_t type, u16_8_t code)
{
    bool handled = false;
    WSYS_LOGI("MultiWidget handleEvent");
#if 0
    for (int i=0;i<m_numchilds;i++) {
        if (m_childs[i]->handleEvent(type,code))
            handled = true;
    }
#else
    if (m_focusWidget) {
        WSYS_LOGI("MultiWidget focus child handleEvent");
        if (m_focusWidget->handleEvent(type,code))
            handled = true;
    }
#endif
    if (!handled)
        handled = handleLocalEvent(type, code);
    return handled;
}

void MultiWidget::setdamage(uint8_t mask)
{
    Widget::setdamage(mask);
    if (mask!=DAMAGE_CHILD) {
        for (int i=0;i<m_numchilds;i++) {
            m_childs[i]->setdamage(mask);
        }
    }
}

void MultiWidget::addChild(Widget *w)
{
    m_childs[m_numchilds++]=w;
    w->setParent(this);
    if (!m_focusWidget) {
        if (w->canFocus()) {
            m_focusWidget = w;
            WSYS_LOGI("Setting focus to child");
            if (hasFocus())
                w->setFocus(true);          // First child grabs focus
        }
    }
    resizeEvent();
    setdamage(DAMAGE_CHILD);
}

MultiWidget::~MultiWidget()
{
    while (m_numchilds--) {
        delete(m_childs[m_numchilds]);
    }
}

void MultiWidget::focusIn()
{
    WSYS_LOGI("MultiWidget Focus IN");
    if (!m_focusWidget)
        m_focusWidget = findNextFocusable(0);

    if (m_focusWidget) {
#ifdef __linux__
        WSYS_LOGI("Focusing %s", typeid(*m_focusWidget).name());

#endif
        m_focusWidget->setFocus(true);
    } else {
        WSYS_LOGI("No child to focus");
    }
}
void MultiWidget::focusOut()
{
    WSYS_LOGI("MultiWidget Focus OUT");
    if (m_focusWidget) {
        m_focusWidget->setFocus(false);
    }
}

int MultiWidget::getChild(Widget *c)
{
    unsigned i;
    for (i=0;i<m_numchilds;i++) {
        if (m_childs[i]==c)
            return i;
    }
    return -1;
}

Widget *MultiWidget::findNextFocusable(int start)
{
    if (m_numchilds==0 || start>=m_numchilds)
        return NULL;

    int i = start;
    do {
        i++;
        if (i==m_numchilds)
            i=0;

        Widget *candidate = m_childs[i];
        if (candidate->canFocus()) {
            return candidate;
        }
    } while (i!=start);
    return NULL;
}

void MultiWidget::focusNext()
{
    WSYS_LOGI("MultiWidget:: Focus next");
    int start = 0;
    if (m_focusWidget) {
        start = getChild(m_focusWidget);
        if (start<0) {
            WSYS_LOGE("Cannot find child!");
            return;
        }
    }
    Widget *next = findNextFocusable(start);
    if (next) {
        if (m_focusWidget)
            m_focusWidget->setFocus(false);
        next->setFocus(true);
        m_focusWidget = next;
    }
}

