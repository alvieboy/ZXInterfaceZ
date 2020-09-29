#include "layout.h"

Layout::Layout(Widget *parent): MultiWidget(parent)
{
    m_spacing = 0;
}

void Layout::addChild(Widget *w) {
    m_flags[m_numchilds] = 0;
    MultiWidget::addChild(w);
}

void Layout::addChild(Widget *w, uint8_t flags) {
    m_flags[m_numchilds] = flags;
    MultiWidget::addChild(w);
}

void Layout::drawImpl()
{
}
