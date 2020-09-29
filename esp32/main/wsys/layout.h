#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include "multiwidget.h"

#define LAYOUT_FLAG_VEXPAND (1<<0)
#define LAYOUT_FLAG_HEXPAND (1<<1)

class Layout: public MultiWidget
{
public:
    Layout(Widget *parent=NULL);

    virtual void drawImpl();

    virtual void resizeEvent() = 0;
    virtual void addChild(Widget *w);
    virtual void addChild(Widget *w, uint8_t flags);
    uint8_t spacing() const { return m_spacing; }
    void setSpacing(uint8_t);
protected:
    uint8_t m_flags[MULTIWIDGET_MAX_CHILDS];
    uint8_t m_spacing;
};

#endif
