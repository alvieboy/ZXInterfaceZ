#include "widget.h"
extern "C" {
#include "esp_log.h"
};

void Widget::resize(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    recalculateScreenPointers();
}

void Widget::resize(uint8_t w, uint8_t h)
{
    m_w = w;
    m_h = h;
    recalculateScreenPointers();
}

void Widget::move(uint8_t x, uint8_t y)
{
    m_x = x;
    m_y = y;
    recalculateScreenPointers();
}

void Widget::draw()
{
    drawImpl();
}

void Widget::recalculateScreenPointers()
{
    m_screenptr.fromxy(m_x, m_y);
    m_attrptr.fromxy(m_x, m_y);

    ESP_LOGI("WSYS", "%p Recomputing xy %d %d", this, m_x, m_y);
    ESP_LOGI("WSYS", "%p Recomputed pointers %04x %04x", this, m_screenptr.getoff(), m_attrptr.getoff() );
}

void Widget::handleEvent(uint8_t type, u16_8_t code)
{
}

Widget::~Widget()
{
    if (m_parent)
        m_parent->removeChild(this);
}

void Widget::removeChild(Widget*)
{

}

Widget::Widget(Widget *parent): m_parent(parent)
{
    m_x = 0;
    m_y = 0;
    m_w = 0;
    m_h = 0;
}