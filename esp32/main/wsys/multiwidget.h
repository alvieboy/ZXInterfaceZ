#include "widgetgroup.h"

#define MULTIWIDGET_MAX_CHILDS 4

class MultiWidget: public WidgetGroup
{
public:
    MultiWidget(Widget *parent): WidgetGroup(parent), m_numchilds(0)
    {
    }

    virtual void resizeEvent() = 0;
    virtual void addChild(Widget *w);
    virtual ~MultiWidget();

    virtual void handleEvent(uint8_t type, u16_8_t code) override;

    virtual void draw(bool force=false) override;
protected:
    uint8_t m_numchilds;
    Widget *m_childs[MULTIWIDGET_MAX_CHILDS];
};

