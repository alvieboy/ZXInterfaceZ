#include "hlayout.h"

HLayout::HLayout(Widget *parent): Layout(parent)
{
}


void HLayout::resizeEvent()
{
#if 0
    unsigned expanded_childs = 0;
    unsigned expanded_size = 0;
    unsigned nonexpanded_minimum_size = 0;


    if (getNumberOfChildren()==0)
        return;

    for (int i=0;i<getNumberOfChildren();i++) {
        if (m_flags[i] & LAYOUT_FLAG_HEXPAND){
            expanded_childs++;
        } else {
            nonexpanded_minimum_size += childAt(i)->getMinimumWidth();
        }
    }

    expanded_size = width() - nonexpanded_minimum_size;

    unsigned mx = x();

    if (expanded_childs) {
        for (int i=0;i<getNumberOfChildren();i++) {
            if (m_flags[i] & LAYOUT_FLAG_HEXPAND) {
                unsigned this_expanded_size = expanded_size / expanded_childs;
                expanded_childs--;
                expanded_size -= this_expanded_size;
                childAt(i)->resize(mx, m_y, this_expanded_size, m_h);
                mx+=this_expanded_size;
            } else {
                unsigned size = childAt(i)->getMinimumWidth();
                childAt(i)->resize(mx, m_y, size , m_h);
                mx+=size;
            }
        }
    } else {
        unsigned dx = Widget::width() / getNumberOfChildren();

        for (int i=0;i<getNumberOfChildren();i++) {
            childAt(i)->resize(mx, m_y, dx, m_w);
            mx+=dx;

        }
    }
#endif
    std::vector<int> sizes;
    computeLayout(sizes,
                  &Widget::getMinimumWidth,
                  LAYOUT_FLAG_HEXPAND,
                  width());

    int mx = x() + m_border;

    for(int index=0;index<getNumberOfChildren();index++) {
        childAt(index)->resize(mx, m_y,sizes[index], m_h);
        mx+=sizes[index]+m_spacing;
    }

}

