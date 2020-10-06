#include "multiwidget.h"

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
    return false;
}

bool MultiWidget::handleEvent(uint8_t type, u16_8_t code)
{
    bool handled = false;
    for (int i=0;i<m_numchilds;i++) {
        if (m_childs[i]->handleEvent(type,code))
            handled = true;
    }
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
    resizeEvent();
    setdamage(DAMAGE_CHILD);
}

MultiWidget::~MultiWidget()
{
    while (m_numchilds--) {
        delete(m_childs[m_numchilds]);
    }
}
