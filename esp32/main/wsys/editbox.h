#ifndef __WSYS_EDITBOX_H__
#define __WSYS_EDITBOX_H__

#include "widget.h"
#include <string>
#include "object_signal.h"

class EditBox: public Widget
{
public:
    EditBox(const char *text=NULL,Widget *parent=NULL);
    virtual void drawImpl();
    void setText(const char *text) { m_text=text; redraw(); }
    const char *getText() const { return m_text.c_str(); }
    virtual bool handleEvent(uint8_t type, u16_8_t code);
    void setEditable(bool e);
    Signal<> &enter() { return m_enter; }
    virtual void focusIn();
    virtual void focusOut();
private:
    std::string m_text;
    bool m_editable;
    Signal<> m_enter;
};


#endif
