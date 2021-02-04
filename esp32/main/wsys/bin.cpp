#include <stdlib.h>
#include "bin.h"
extern "C" {
#include "esp_log.h"
}

Bin::Bin(Widget *parent): WidgetGroup(parent), m_child(NULL)
{
}

void Bin::focusIn()
{
    WSYS_LOGI("Focus IN child %p", m_child);
#ifdef __linux__
    WSYS_LOGI(" > %s", typeid(*m_child).name());
#endif
    if (m_child)
        m_child->setFocus(true);
}
void Bin::focusOut()
{
    WSYS_LOGI("Focus OUT child");
    if (m_child)
        m_child->setFocus(false);
}


void Bin::setChild(Widget *c)
{
    m_child = c;
    c->setParent(this);
    WSYS_LOGI( "resize due to addchild");
    resizeEvent();
    setdamage(DAMAGE_CHILD);
}

bool Bin::handleEvent(uint8_t type, u16_8_t code)
{
    if (m_child)
        return m_child->handleEvent(type, code);
    return false;
}

void Bin::draw(bool force)
{
    WSYS_LOGI( "Bin::draw force=%d damage=0x%02x\n", force?1:0, damage());

    if (force || (damage() & ~DAMAGE_CHILD)) { // If any bits beside child, then draw
        WSYS_LOGI( "Bin::draw impl");
        drawImpl();
    }


    if (m_child) {
        WSYS_LOGI( "Bin::draw child force %d", force?1:0);
        if (force || (damage() & DAMAGE_CHILD))  {
            WSYS_LOGI( "Bin::draw child force=%d child_damage=0x%02x\n", force?1:0, m_child->damage());

            if (force || m_child->damage())
                m_child->draw(force);
            m_child->clear_damage();
        }
    } else {
        WSYS_LOGI( "Bin::draw no child");
    }
    clear_damage();

}

Bin::~Bin()
{
    if (m_child) {
        WSYS_LOGI("Deleting child %p", m_child);
        delete(m_child);
    }
}

void Bin::removeChild(Widget *c)
{
    if (c==m_child) {
        delete(m_child);
        m_child = NULL;
    }
}


