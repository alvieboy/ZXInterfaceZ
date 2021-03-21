#ifndef __FIXEDLAYOUT_H__
#define __FIXEDLAYOUT_H__

#include "multiwidget.h"

class FixedLayout: public MultiWidget
{
public:
    FixedLayout(Widget *parent=NULL);

    virtual void drawImpl() override {};

    virtual void resizeEvent() override;
    virtual void addChild(Widget *widget, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
protected:
    uint8_t m_childx[MULTIWIDGET_MAX_CHILDS];
    uint8_t m_childy[MULTIWIDGET_MAX_CHILDS];
    uint8_t m_childw[MULTIWIDGET_MAX_CHILDS];
    uint8_t m_childh[MULTIWIDGET_MAX_CHILDS];
};

#endif
