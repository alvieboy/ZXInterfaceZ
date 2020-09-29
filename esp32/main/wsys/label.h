#ifndef __WSYS_LABEL_H__
#define __WSYS_LABEL_H__

#include "widget.h"

class Label: public Widget
{
public:
    Label(const char *text=NULL,Widget *parent=NULL);
    virtual void drawImpl();
    void setText(const char *text) { m_text=text; redraw(); }
private:
    const char *m_text;
};


#endif
