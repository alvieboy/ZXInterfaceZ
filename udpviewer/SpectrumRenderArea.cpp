#include "SpectrumRenderArea.h"

#define BORDER_TB 8
#define BORDER_LR 8

SpectrumRenderArea::SpectrumRenderArea(QWidget *w): RenderArea(256+(BORDER_LR*2), 192+ (BORDER_TB*2), w)
{
    m_flashinvert = false;
    m_flashtimer = new QTimer(this);
    m_flashtimer->setSingleShot(false);
    connect(m_flashtimer, &QTimer::timeout, this, &SpectrumRenderArea::onFlashTimerExpired);
    m_flashtimer->start(1000);
    initColors();
}

void SpectrumRenderArea::onFlashTimerExpired()
{
    m_flashinvert = !m_flashinvert;
    render(true);
}

void SpectrumRenderArea::initColors()
{
    normal_colors[0] = 0x000000;
    normal_colors[1] = 0x0000D7;
    normal_colors[2] = 0xD70000;
    normal_colors[3] = 0xD700D7;
    normal_colors[4] = 0x00D700;
    normal_colors[5] = 0x00D7D7;
    normal_colors[6] = 0xD7D700;
    normal_colors[7] = 0xD7D7D7;

    bright_colors[0] = 0x000000;
    bright_colors[1] = 0x0000FF;
    bright_colors[2] = 0xFF0000;
    bright_colors[3] = 0xFF00FF;
    bright_colors[4] = 0x00FF00;
    bright_colors[5] = 0x00FFFF;
    bright_colors[6] = 0xFFFF00;
    bright_colors[7] = 0xFFFFFF;
};

void SpectrumRenderArea::renderSCR(const uint8_t *data)
{
    memcpy(m_scr,data, SPECTRUM_FRAME_SIZE);
    render();
}

void SpectrumRenderArea::render(bool flashonly)
{
    int x;
    int y;
    uint32_t fg, bg, t;
    int flash;

    for (y=0;y<192;y++) {
        for (x=0; x<32; x++) {
            unsigned offset = x; // 5 bits

            offset |= ((y>>3) & 0x7)<<5;  // Y5, Y4, Y3
            offset |= (y & 7)<<8;         // Y2, Y1, Y0
            offset |= ((y>>6) &0x3 ) << 11;               // Y8, Y7

            uint8_t attr = getattr(m_scr, x, y>>3);
            parseattr( attr, &fg, &bg, &flash);

            if (!flash && flashonly)
                continue;

            if (flash && m_flashinvert) {
                t = bg;
                bg = fg;
                fg = t;
            }
            // Get m_scr
            uint8_t pixeldata8 = m_scr[offset];
            //printf("%d %d Pixel %02x attr %02x fg %08x bg %08x\n", x, y, pixeldata8, attr, fg, bg);
            for (int ix=0;ix<8;ix++) {
                drawPixel(BORDER_LR + (x*8 + ix),
                          BORDER_TB+y,
                          pixeldata8 & 0x80 ? fg: bg);
                pixeldata8<<=1;
            }
        }
    }
}

uint32_t SpectrumRenderArea::normal_colors[8];
uint32_t SpectrumRenderArea::bright_colors[8];
