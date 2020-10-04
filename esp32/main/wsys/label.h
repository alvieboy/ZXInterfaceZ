#ifndef __WSYS_LABEL_H__
#define __WSYS_LABEL_H__

#include "widget.h"
#include <string>

class Label: public Widget
{
public:
    Label(const char *text=NULL,Widget *parent=NULL);
    virtual void drawImpl();
    void setText(const char *text) { m_text=text; redraw(); }
    void setBackground(uint8_t background) { m_background=background; redraw(); }
    void setSpacing(uint8_t spacing) { m_spacing = spacing; redraw(); }
    const char *getText() const { return m_text.c_str(); }
private:
    std::string m_text;
    int16_t m_background;
    uint8_t m_spacing;
};


#endif
