#include "widget.h"

class WidgetGroup: public Widget
{
public:
    WidgetGroup(Widget *parent=NULL);
    virtual void resizeEvent() = 0;
};


