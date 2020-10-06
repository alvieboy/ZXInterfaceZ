#ifndef __WSYS_VLAYOUT_H__
#define __WSYS_VLAYOUT_H__

#include "layout.h"

class VLayout: public Layout
{
public:
    VLayout(Widget *parent=NULL);
    virtual void resizeEvent();
protected:
};

#endif
