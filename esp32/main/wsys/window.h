#ifndef __WSYS_WINDOW_H__
#define __WSYS_WINDOW_H__

#include "bin.h"
#include "helpdisplayer.h"

class Window: public Bin, public HelpDisplayer
{
public:
    Window(const char *title, uint8_t w, uint8_t h);
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

    void destroy() {
        screen__removeWindow(this);
    }
    virtual int availableWidth() const override { return width()-(2*m_border); }
    virtual int availableHeight() const override { return height()-(2*m_border); }

    virtual void focusNextPrev(bool next);
    virtual void setChild(Widget *c);
    virtual void setVisible(bool);
    virtual bool handleEvent(uint8_t type, u16_8_t code);

    void setFocusKeys(uint8_t next, uint8_t prev);

    virtual void visibilityOrFocusChanged(Widget *w);

protected:
    virtual ~Window();
    void dumpFocusTree();
    friend void screen__check_redraw();
    friend void screen__destroyAll();
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
    uint8_t m_focusnextkey;
    uint8_t m_focusprevkey;
    const char *m_helptext;
    const char *m_statustext;
    Widget *m_focusWidget;
};

#endif
