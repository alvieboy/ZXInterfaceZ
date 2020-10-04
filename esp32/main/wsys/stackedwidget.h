#ifndef __WSYS_STACKEDWIDGET_H__
#define __WSYS_STACKEDWIDGET_H__

#include "multiwidget.h"

class StackedWidget: public MultiWidget
{
public:
    StackedWidget(Widget *parent=NULL): MultiWidget(parent)
    {
        m_currentindex = 0;
    }

    virtual bool handleEvent(uint8_t type, u16_8_t code) override;
    virtual void draw(bool force=false) override;
    virtual void setCurrentIndex(uint8_t index);
    virtual void drawImpl();
    virtual void resizeEvent();

protected:
    uint8_t m_currentindex;
};

#endif
