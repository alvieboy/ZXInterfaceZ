#ifndef __WSYS_WINDOW_H__
#define __WSYS_WINDOW_H__

#include "bin.h"
#include "helpdisplayer.h"

class Window: public Bin, public HelpDisplayer
{
public:
    Window(const char *title, uint8_t w, uint8_t h);
    ~Window();
    // Virtual methods
    virtual void resizeEvent() override;
    void setTitle(const char *title);
    void setBorder(uint8_t border);
    bool needRedraw();
    void setWindowHelpText(const char *);
    void setStatusLines(uint8_t lines);

    void displayHelpText(const char *c) override;
    virtual void draw(bool force=false) override;
    void clearChildArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

protected:
    void fillHeaderLine(attrptr_t attr);
    void setBackground();
    virtual void drawImpl() override;
    void drawWindowCore();
    void drawStatus();
    bool hasHelpText() const { return m_helptext!=NULL; }
    int  statusLines() const { return m_statuslines; }

    const char *m_title;
    uint8_t m_border;
    uint8_t m_statuslines;
    const char *m_helptext;
    const char *m_statustext;
};

#endif
