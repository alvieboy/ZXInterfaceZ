#include "vlayout.h"

VLayout::VLayout(Widget *parent): Layout(parent)
{
}

void VLayout::resizeEvent()
{
    std::vector<int> sizes;
    computeLayout(sizes,
                  &Widget::getMinimumHeight,
                  LAYOUT_FLAG_VEXPAND,
                  height());

    int my = y() + m_border;

    for(int index=0;index<getNumberOfChildren();index++) {
        childAt(index)->resize(m_x, my, m_w, sizes[index]);
        my+=sizes[index]+m_spacing;
    }
}

