#include "multiwidget.h"

#define LAYOUT_FLAG_VEXPAND (1<<0)

class VLayout: public MultiWidget
{
public:
    VLayout(Widget *parent=NULL);

    virtual void drawImpl();

    virtual void resizeEvent();
    virtual void addChild(Widget *w);
    virtual void addChild(Widget *w, uint8_t flags);

protected:
    uint8_t m_flags[MULTIWIDGET_MAX_CHILDS];
};
