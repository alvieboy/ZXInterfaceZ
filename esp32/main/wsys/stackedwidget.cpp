#include "stackedwidget.h"

bool StackedWidget::handleEvent(uint8_t type, u16_8_t code)
{
    if (m_currentindex >= m_numchilds)
        return false;
    return m_childs[m_currentindex]->handleEvent(type, code);
}

void StackedWidget::draw(bool force)
{
    if (m_currentindex >= m_numchilds)
        return;
    m_childs[m_currentindex]->draw(force);
}

void StackedWidget::setCurrentIndex(uint8_t index)
{
    m_currentindex = index;
    // force redraw
    m_childs[index]->redraw();

    //draw(true);
}

void StackedWidget::drawImpl()
{
}


void StackedWidget::resizeEvent()
{
    for (int i=0; i<m_numchilds;i++) {
        m_childs[i]->resize(m_x, m_y, m_w, m_h);
    }
}
