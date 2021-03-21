#ifndef __WSYS_BIN_H__
#define __WSYS_BIN_H__

#include "widgetgroup.h"

class Bin: public WidgetGroup
{
public:
    Bin(Widget *parent=NULL);
    virtual ~Bin();
    virtual void draw(bool force=false) override;
    virtual void removeChild(Widget *c) override;
    virtual bool handleEvent(wsys_input_event_t) override;
    virtual void focusIn() override;
    virtual void focusOut() override;
    virtual void setVisible(bool) override;

    virtual void setChild(Widget *);
protected:

    Widget *m_child;
};



#endif
