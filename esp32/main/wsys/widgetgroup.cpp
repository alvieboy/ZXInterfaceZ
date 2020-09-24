#include "widgetgroup.h"

WidgetGroup::WidgetGroup(Widget *parent): Widget(parent)
{
}

void WidgetGroup::resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    ESP_LOGI( "WSYS", "resize");
    Widget::resize(x,y,w,h);
    resizeEvent();
}

void WidgetGroup::move(uint8_t x, uint8_t y)
{
    Widget::move(x,y);
    resizeEvent();
}

