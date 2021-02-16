#ifndef __WSYS_MULTIWIDGET_H__
#define __WSYS_MULTIWIDGET_H__

#include "widgetgroup.h"
#include <vector>

#define MULTIWIDGET_MAX_CHILDS 12

class MultiWidget: public WidgetGroup
{
public:
    MultiWidget(Widget *parent): WidgetGroup(parent)
    {
    }

    virtual void resizeEvent() = 0;
    virtual void addChild(Widget *w);
    virtual ~MultiWidget();

    virtual bool handleEvent(uint8_t type, u16_8_t code) override;
    virtual bool handleLocalEvent(uint8_t type, u16_8_t code);

    virtual void draw(bool force=false) override;
    virtual void setdamage(uint8_t mask);
    virtual void focusIn() override;
    virtual void focusOut() override;
    virtual bool canFocus() const;
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
