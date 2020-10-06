#ifndef __WSYS_HLAYOUT_H__
#define __WSYS_HLAYOUT_H__

#include "layout.h"

class HLayout: public Layout
{
public:
    HLayout(Widget *parent=NULL);
    virtual void resizeEvent();
};

#endif
