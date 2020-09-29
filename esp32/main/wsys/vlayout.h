#include "layout.h"

class VLayout: public Layout
{
public:
    VLayout(Widget *parent=NULL);
    virtual void resizeEvent();
protected:
};
