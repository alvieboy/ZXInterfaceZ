#ifndef __WSYS_BUTTON_H__
#define __WSYS_BUTTON_H__

#include "widget.h"
#include <string.h>

class Button: public Widget
{
public:
    Button(Widget *parent=NULL, const char *text=NULL);
    virtual void drawImpl();
    void setText(const char *text) { m_text=text; m_textlen=strlen(text); redraw(); }
    virtual void handleEvent(uint8_t type, u16_8_t code);
    void onClick(void (*callback)(void*),void*data) {
        m_onclick=callback;
        m_onclickdata=data;
    }
private:
    const char *m_text;
    uint8_t m_textlen;
    void (*m_onclick)(void*);
    void *m_onclickdata;
};


#endif
