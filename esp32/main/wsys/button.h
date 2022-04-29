#pragma once

#include "widget.h"
#include <string.h>
#include "object_signal.h"

class Button: public Widget
{
public:
    Button(const char *text, Widget *parent=NULL);
    virtual void drawImpl() override;
    void setText(const char *text) { m_text=text; m_textlen=strlen(text); redraw(); }
    virtual bool handleEvent(wsys_input_event_t) override;
    Signal<> &clicked() { return m_clicked; }
    void setAccelKey(char c) { m_accel=c; }
    void setSpacing(uint8_t s) { m_spacing=s; redraw(); }
    virtual void focusIn() override;
    virtual void focusOut() override;
    void setThumbFont(bool yes);
private:
    const char *m_text;
    uint8_t m_textlen;
    uint8_t m_spacing;
    bool m_thumb;
    int m_accel;
    Signal<> m_clicked;
};
