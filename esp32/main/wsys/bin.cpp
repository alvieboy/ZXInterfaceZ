#include <stdlib.h>
#include "bin.h"
extern "C" {
#include "esp_log.h"
}

Bin::Bin(Widget *parent): WidgetGroup(parent), m_child(NULL)
{
}

void Bin::resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    ESP_LOGI( "WSYS", "resize");
    Widget::resize(x,y,w,h);
    resizeEvent();
}

void Bin::move(uint8_t x, uint8_t y)
{
    Widget::move(x,y);
    resizeEvent();
}

void Bin::setChild(Widget *c)
{
    m_child = c;
    c->setParent(this);
    ESP_LOGI( "WSYS", "resize due to addchild");

    resizeEvent();
}

void Bin::handleEvent(uint8_t type, u16_8_t code)
{
    if (m_child)
        m_child->handleEvent(type, code);
}

void Bin::draw()
{
    drawImpl();
    if (m_child)
        m_child->draw();
}

Bin::~Bin()
{
    if (m_child)
        delete(m_child);
}

void Bin::removeChild(Widget *c)
{
    if (c==m_child) {
        //delete(m_child);
        m_child = NULL;
    }
}
