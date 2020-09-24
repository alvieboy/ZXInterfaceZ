#include "vlayout.h"

VLayout::VLayout(Widget *parent): MultiWidget(parent)
{
}


/*
 1
 4 (exp)
 3 (exp)
 1
 1

 height=10
 expanded_childs=2
 expanded_size = 10 - (childs-expanded_childs) = 7
 expanded_delta = 7/2;
 */

void VLayout::resizeEvent()
{
    unsigned expanded_childs = 0;
    unsigned expanded_size = 0;

    if (m_numchilds==0)
        return;

    for (int i=0;i<m_numchilds;i++) {
        if (m_flags[i] & LAYOUT_FLAG_VEXPAND)
            expanded_childs++;
    }

    expanded_size = height()-(m_numchilds-expanded_childs);

    unsigned my = y();

    if (expanded_childs) {
        for (int i=0;i<m_numchilds;i++) {
            if (m_flags[i] & LAYOUT_FLAG_VEXPAND) {
                unsigned this_expanded_size = expanded_size / expanded_childs;
                expanded_childs--;
                expanded_size -= this_expanded_size;
                m_childs[i]->resize(m_x, my, m_w, this_expanded_size);
                my+=this_expanded_size;
            } else {
                m_childs[i]->resize(m_x, my, m_w, 1);
                my++;
            }
        }
    } else {
        unsigned dy = Widget::height() / m_numchilds;

        for (int i=0;i<m_numchilds;i++) {
            m_childs[i]->resize(m_x, my, m_w, dy);
            my+=dy;

        }
    }
}

void VLayout::drawImpl()
{
}


void VLayout::addChild(Widget *w) {
    m_flags[m_numchilds] = 0;
    MultiWidget::addChild(w);
}

void VLayout::addChild(Widget *w, uint8_t flags) {
    m_flags[m_numchilds] = flags;
    MultiWidget::addChild(w);
}
