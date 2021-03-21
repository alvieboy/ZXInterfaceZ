#ifndef __WSYS_MULTIWIDGET_H__
#define __WSYS_MULTIWIDGET_H__

#include "widgetgroup.h"
#include <vector>

#define MULTIWIDGET_MAX_CHILDS 24

class MultiWidget: public WidgetGroup
{
public:
    MultiWidget(Widget *parent): WidgetGroup(parent)
    {
    }

    virtual void resizeEvent() override = 0;
    virtual void addChild(Widget *w);
    virtual ~MultiWidget();

    virtual bool handleEvent(wsys_input_event_t) override;
    virtual bool handleLocalEvent(wsys_input_event_t);

    virtual void draw(bool force=false) override;
    virtual void setdamage(uint8_t mask) override;
    virtual void focusIn() override;
    virtual void focusOut() override;
    virtual bool canFocus() const override;
    virtual void setFocus(bool focus) override;
    virtual int getNumberOfChildren() const { return m_childs.size(); }
    virtual int getNumberOfVisibleChildren() const;
    virtual void setVisible(bool) override;
protected:
    int getChild(Widget *c);
    Widget *childAt(int index) { return m_childs[index]; }
    Widget *lastChild() {
        if (!m_childs.size()) return NULL;
        return m_childs[ m_childs.size()-1 ];
    }

    const Widget *childAt(int index) const { return m_childs[index]; }

    std::vector<Widget*> &childs() { return m_childs; }

private:
    std::vector<Widget*> m_childs;
};

#endif
