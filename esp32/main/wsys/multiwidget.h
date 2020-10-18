#ifndef __WSYS_MULTIWIDGET_H__
#define __WSYS_MULTIWIDGET_H__

#include "widgetgroup.h"

#define MULTIWIDGET_MAX_CHILDS 12

class MultiWidget: public WidgetGroup
{
public:
    MultiWidget(Widget *parent): WidgetGroup(parent), m_numchilds(0)
    {
    }

    virtual void resizeEvent() = 0;
    virtual void addChild(Widget *w);
    virtual ~MultiWidget();

    virtual bool handleEvent(uint8_t type, u16_8_t code) override;
    virtual bool handleLocalEvent(uint8_t type, u16_8_t code);

    virtual void draw(bool force=false) override;
    virtual void setdamage(uint8_t mask);

protected:
    uint8_t m_numchilds;
    Widget *m_childs[MULTIWIDGET_MAX_CHILDS];
};

#endif
