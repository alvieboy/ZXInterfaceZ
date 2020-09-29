#include "vlayout.h"

VLayout::VLayout(Widget *parent): Layout(parent)
{
}

void VLayout::resizeEvent()
{
    unsigned expanded_childs = 0;
    unsigned expanded_size = 0;
    unsigned nonexpanded_minimum_size = 0;

    if (m_numchilds==0)
        return;

    for (int i=0;i<m_numchilds;i++) {
        if (m_flags[i] & LAYOUT_FLAG_VEXPAND) {
            expanded_childs++;
        } else {
            nonexpanded_minimum_size += m_childs[i]->getMinimumHeight();
        }
    }

    expanded_size = height() - nonexpanded_minimum_size;

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
                unsigned size = m_childs[i]->getMinimumHeight();
                m_childs[i]->resize(m_x, my, m_w,size);
                my+=size;
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

