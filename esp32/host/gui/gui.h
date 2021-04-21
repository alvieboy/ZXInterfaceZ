#ifndef __GUI_H__
#define __GUI_H__

#include "SpectrumWidget.h"
#include "QtSpecem.h"
#include <QDateTime>
#include "qgifimage.h"

class QImage;
class QPushButton;

class GuiWindow: public EmulatorWindow
{
public:
    GuiWindow();
    void initGifSave();
    void stopGif();
    int setupui(const QString&romfilename,int sock,bool trace_set, unsigned trace_address);
public slots:
    void onPaintCompleted(QImage&);
    void onGifButtonPressed();
private:
    QImage m_lastImage;
    QDateTime m_lastImageTime;
    QGifImage *m_gif;
    SpectrumWidget *m_spectrumwidget;
    QPushButton *gif_button;
};

#endif
