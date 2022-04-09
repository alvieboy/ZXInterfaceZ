#pragma once

#include "multiwidget.h"
#include <vector>

#define LAYOUT_FLAG_VEXPAND (1<<0)
#define LAYOUT_FLAG_HEXPAND (1<<1)

class Layout: public MultiWidget
{
public:
    Layout(Widget *parent=NULL);

    virtual void drawImpl() override;

    virtual void resizeEvent() override = 0;
    virtual void addChild(Widget *w, uint8_t flags=0);
    uint8_t spacing() const { return m_spacing; }
    void setSpacing(uint8_t);
    uint8_t border() const { return m_border; }
    void setBorder(uint8_t);

    void computeLayout(std::vector<int> &sizes,
                       uint8_t (Widget::*sizefun)(void) const,
                       uint8_t flag,
                       int size);

    virtual void visibilityOrFocusPolicyChanged(Widget *w) override;

protected:
    std::vector<uint8_t> m_flags;
    uint8_t m_spacing;
    uint8_t m_border;
};
