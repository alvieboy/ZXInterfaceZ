#include "layout.h"

class HLayout: public Layout
{
public:
    HLayout(Widget *parent=NULL);
    virtual void resizeEvent();
};
