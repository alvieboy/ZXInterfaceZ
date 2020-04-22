#ifndef __SPECTRUMRENDERAREA_H__
#define __SPECTRUMRENDERAREA_H__

#include "RenderArea.h"
#include <inttypes.h>

#define SPECTRUM_FRAME_SIZE 6912

class SpectrumRenderArea: public RenderArea
{
public:
    SpectrumRenderArea(QWidget *parent);
    void renderSCR(const uint8_t *data);

    void setFPS(int fps) {
        m_fps=fps;
        if (fps==0)
            finishFrame();
    }

public slots:
    void onFlashTimerExpired();
protected:
    static void initColors();

    static uint32_t parsecolor( uint8_t col, int bright)
    {
        return bright ? bright_colors[col] : normal_colors[col];
    }

    static void parseattr( uint8_t attr, uint32_t *fg, uint32_t *bg, int *flash)
    {
        *fg = parsecolor(attr & 0x7, attr & 0x40);
        *bg = parsecolor((attr>>3) & 0x7, attr & 0x40);
        *flash = attr & 0x80;
    }

    static uint8_t getattr(const uint8_t *data, int x, int y)
    {
        return data[ (32*192) + x + (y*32) ];
    }
    void render(bool flashonly=false);


    virtual void finaliseImage() override;

private:
    static uint32_t normal_colors[8];
    static uint32_t bright_colors[8];
    uint8_t m_scr[SPECTRUM_FRAME_SIZE];

    QTimer *m_flashtimer;
    QFont m_fpsfont;
    bool m_flashinvert;
    int m_fps;

};

#endif
