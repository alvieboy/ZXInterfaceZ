#include "widget.h"

class VBar: public Widget
{
public:
    VBar(Widget*parent=NULL): Widget(parent)
    {
        redraw();
    }
    virtual void drawImpl() override;
};
