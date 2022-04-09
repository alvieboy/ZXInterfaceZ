#pragma once

#include "widget.h"
#include <string>
#include "object_signal.h"

class EditBox: public Widget
{
public:
    EditBox(const char *text=NULL,Widget *parent=NULL);
    virtual void drawImpl() override;
    void setText(const char *text) { m_text=text; redraw(); }
    const char *getText() const { return m_text.c_str(); }
    virtual bool handleEvent(wsys_input_event_t) override;
    void setEditable(bool e);
    Signal<> &enter() { return m_enter; }
    virtual void focusIn() override;
    virtual void focusOut() override;
private:
    std::string m_text;
    bool m_editable;
    Signal<> m_enter;
};
