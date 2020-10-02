#ifndef __WSYS_WIDGETGROUP_H__
#define __WSYS_WIDGETGROUP_H__

#include "widget.h"

class WidgetGroup: public Widget
{
public:
    WidgetGroup(Widget *parent=NULL);
    virtual void resizeEvent() = 0;
    void resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    void move(uint8_t x, uint8_t y);
};


#endif
