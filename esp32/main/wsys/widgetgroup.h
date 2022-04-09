#pragma once

#include "widget.h"

class WidgetGroup: public Widget
{
public:
    WidgetGroup(Widget *parent=NULL);
    virtual void resizeEvent() = 0;
    virtual void resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h) override;
    virtual void move(uint8_t x, uint8_t y) override;

    virtual int availableWidth() const { return width(); };
    virtual int availableHeight() const { return height(); }

};
\
