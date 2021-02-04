#ifndef __WSYS_FRAME_H__
#define __WSYS_FRAME_H__

#include "bin.h"

class Frame: public Bin
{
public:
    Frame(const char *title, uint8_t w, uint8_t h, bool drawbackground=false);
    // Virtual methods
    virtual void resizeEvent() override;
    void setTitle(const char *title);
    void setBorder(uint8_t border);
    bool needRedraw();
    virtual void draw(bool force=false) override;
    virtual void setChild(Widget *w) { Bin::setChild(w); }
    void clearChildArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

protected:
    void drawFrame();
    virtual ~Frame();
    virtual void drawImpl() override;
    const char *m_title;
    uint8_t m_border;
    bool m_drawbackground;
};

#endif
