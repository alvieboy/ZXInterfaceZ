#ifndef __WSYS_LABEL_H__
#define __WSYS_LABEL_H__

#include "widget.h"

class Label: public Widget
{
public:
    Label(Widget *parent=NULL, const char *text=NULL);
    virtual void drawImpl();
    void setText(const char *text) { m_text=text; redraw(); }
private:
    const char *m_text;
};


#endif
