#include "hlayout.h"

HLayout::HLayout(Widget *parent): Layout(parent)
{
}


void HLayout::resizeEvent()
{
    unsigned expanded_childs = 0;
    unsigned expanded_size = 0;
    unsigned nonexpanded_minimum_size = 0;


    if (m_numchilds==0)
        return;

    for (int i=0;i<m_numchilds;i++) {
        if (m_flags[i] & LAYOUT_FLAG_HEXPAND){
            expanded_childs++;
        } else {
            nonexpanded_minimum_size += m_childs[i]->getMinimumWidth();
        }
    }

    expanded_size = width() - nonexpanded_minimum_size;

    unsigned mx = x();

    if (expanded_childs) {
        for (int i=0;i<m_numchilds;i++) {
            if (m_flags[i] & LAYOUT_FLAG_HEXPAND) {
                unsigned this_expanded_size = expanded_size / expanded_childs;
                expanded_childs--;
                expanded_size -= this_expanded_size;
                m_childs[i]->resize(mx, m_y, this_expanded_size, m_h);
                mx+=this_expanded_size;
            } else {
                unsigned size = m_childs[i]->getMinimumWidth();
                m_childs[i]->resize(mx, m_y, size , m_h);
                mx+=size;
            }
        }
    } else {
        unsigned dx = Widget::width() / m_numchilds;

        for (int i=0;i<m_numchilds;i++) {
            m_childs[i]->resize(mx, m_y, dx, m_w);
            mx+=dx;

        }
    }
}

