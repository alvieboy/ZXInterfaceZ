#include "fixedlayout.h"

FixedLayout::FixedLayout(Widget *parent): MultiWidget(parent)
{
}

void FixedLayout::resizeEvent()
{
    for (int i=0;i<getNumberOfChildren();i++) {
        int w = m_childw[i];
        int h = m_childw[i];

        if ((m_childx[i] + w)>width()) {
        }
        if ((m_childy[i] + h)>height()) {
        }

        childAt(i)->resize(m_x + m_childx[i],
                            m_y + m_childy[i],
                            m_childw[i],
                            m_childh[i]);
    }
}


void FixedLayout::addChild(Widget *widget, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    MultiWidget::addChild(widget);
    m_childx[getNumberOfChildren()-1] = x;
    m_childy[getNumberOfChildren()-1] = y;
    m_childw[getNumberOfChildren()-1] = w;
    m_childh[getNumberOfChildren()-1] = h;
}
