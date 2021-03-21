#ifndef __WSYS_STACKEDWIDGET_H__
#define __WSYS_STACKEDWIDGET_H__

#include "multiwidget.h"

class StackedWidget: public MultiWidget
{
public:
    StackedWidget(Widget *parent=NULL): MultiWidget(parent)
    {
        m_currentindex = -1;
    }

    virtual bool handleEvent(wsys_input_event_t) override;
    virtual void draw(bool force=false) override;
    virtual void setCurrentIndex(uint8_t index);
    virtual void drawImpl() override;
    virtual void resizeEvent() override;
    virtual void addChild(Widget *w) override;


protected:
    int8_t m_currentindex;
};

#endif
