#ifndef __RENDERAREA_H__
#define __RENDERAREA_H__

#include <QWidget>
#include <QVideoWidget>
#include <QImage>
#include <QTimer>
#include "ScreenDrawer.h"

class RenderArea : public QVideoWidget, public ScreenDrawer
{
    Q_OBJECT

public:
    RenderArea(unsigned w, unsigned h, QWidget *parent = 0);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void startFrame();
    void finishFrame();
    void setVideoMode(bool) override;
protected:
    void paintEvent(QPaintEvent *event) override;
    void drawPixel(int x, int y, uint32_t color) override;
    void drawImage(int x, int y, QImage *);
    void drawHLine(int x, int y, int width, uint32_t color);
    void drawVLine(int x, int y, int width, uint32_t color);

    QVideoWidget* getVideoWidget() override;


    QImage *image;
    bool videomode;
    unsigned m_width;
    unsigned m_height;
};

#endif
