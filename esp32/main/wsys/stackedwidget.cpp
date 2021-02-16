
#include "stackedwidget.h"

bool StackedWidget::handleEvent(uint8_t type, u16_8_t code)
{
    if (m_currentindex >= getNumberOfChildren())
        return false;
    return childAt(m_currentindex)->handleEvent(type, code);
}

void StackedWidget::addChild(Widget *w)
{
    if (m_currentindex<0) {
        w->show();
        m_currentindex = 0;
    } else {
        w->hide();
    }
    MultiWidget::addChild(w);
}

void StackedWidget::draw(bool force)
{
    if (m_currentindex >= getNumberOfChildren())
        return;
    childAt(m_currentindex)->draw(force);
}

void StackedWidget::setCurrentIndex(uint8_t index)
{
    if (m_currentindex<getNumberOfChildren()) {
        //childAt(m_currentindex)->setFocus(false);
        childAt(m_currentindex)->hide();
    }
    m_currentindex = index;
    // force redraw
    //childAt(m_currentindex)->setFocus(hasFocus());
    childAt(index)->show();
    childAt(index)->redraw();

    //draw(true);
}

void StackedWidget::drawImpl()
{
}


void StackedWidget::resizeEvent()
{
    for (int i=0; i<getNumberOfChildren();i++) {
        childAt(i)->resize(m_x, m_y, m_w, m_h);
    }
}

