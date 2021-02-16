#ifndef __WSYS_BIN_H__
#define __WSYS_BIN_H__

#include "widgetgroup.h"

class Bin: public WidgetGroup
{
public:
    Bin(Widget *parent=NULL);
    virtual ~Bin();
    virtual void draw(bool force=false) override;
    void removeChild(Widget *c);
    virtual bool handleEvent(uint8_t type, u16_8_t code) override;
    virtual void focusIn();
    virtual void focusOut();
    virtual void setVisible(bool) override;

    virtual void setChild(Widget *);
protected:

    Widget *m_child;
};



#endif
