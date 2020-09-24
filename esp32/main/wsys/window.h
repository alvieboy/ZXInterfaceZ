#ifndef __WSYS_WINDOW_H__
#define __WSYS_WINDOW_H__

#include "bin.h"

class Window: public Bin
{
public:
    Window(const char *title, uint8_t w, uint8_t h);
    ~Window();
    // Virtual methods
    virtual void resizeEvent() override;
    void setTitle(const char *title);
    void setBorder(uint8_t border);
    bool needRedraw();
    void setHelpText(const char *);

protected:
    void fillHeaderLine(attrptr_t attr);
    void setBackground();
    virtual void drawImpl() override;
    bool hasHelpText() const { return m_helptext!=NULL; }
    void setBGLine(attrptr_t attrptr, uint8_t value);

    const char *m_title;
    uint8_t m_border;
    const char *m_helptext;
};

#endif
