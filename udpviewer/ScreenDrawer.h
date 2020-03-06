#ifndef __SCREENDRAWER_H__
#define __SCREENDRAWER_H__

#include <inttypes.h>
class QVideoWidget;
class QImage;

class ScreenDrawer
{
public:
    virtual void drawPixel(int x, int y, uint32_t color)=0;
    virtual void drawImage(int x, int y, QImage *) = 0;
    virtual void drawHLine(int x, int y, int width, uint32_t color) = 0;
    virtual void drawVLine(int x, int y, int width, uint32_t color) = 0;
    virtual void startFrame()=0;
    virtual void finishFrame()=0;
    virtual QVideoWidget* getVideoWidget() =0;
    virtual void setVideoMode(bool) = 0;

};

#endif
