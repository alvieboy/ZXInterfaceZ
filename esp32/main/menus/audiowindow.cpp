#include "slider.h"
#include "window.h"
#include "audiowindow.h"
#include "audio.h"
#include "fixedlayout.h"
#include "button.h"

class AudioWidget: public FixedLayout
{
public:
    AudioWidget();
    virtual void drawImpl();
private:
    FloatSlider *m_volume[4];
    FloatSlider *m_balance[4];
    FixedLayout *m_layout;
    Button *m_button;
};

AudioWindow::AudioWindow(): Window("Audio settings", 22, 22)
{
    m_audiowidget = WSYSObject::create<AudioWidget>();
    setChild(m_audiowidget);
    setWindowHelpText("Use upper key for fine-tuning");
}

AudioWidget::AudioWidget()
{
    float v, b;
    unsigned i;
    const char accels[4][4] = {
        { 'w','e','r','t' },
        { 'y','u','i','o' },
        { 's','d','f','g' },
        { 'h','j','k','l' },
    };
    const char uaccels[4][4] = {
        { 'W','E','R','T' },
        { 'Y','U','I','O' },
        { 'S','D','F','G' },
        { 'H','J','K','L' },
    };

    for (i=0;i<4; i++) {

        m_volume[i] = WSYSObject::create<FloatSlider>();
        m_balance[i] = WSYSObject::create<FloatSlider>();

        m_volume[i]->setMinMax(0.0F, 1.0F);
        m_volume[i]->setAccelMinor(uaccels[i][0],uaccels[i][1],0.01F);
        m_volume[i]->setAccelMajor(accels[i][0],accels[i][1],0.1F);

        m_balance[i]->setMinMax(-1.0F, 1.0F);
        m_balance[i]->setAccelMinor(uaccels[i][2],uaccels[i][3],0.01F);
        m_balance[i]->setAccelMajor(accels[i][2],accels[i][3],0.1F);

        audio__get_volume_f(i, &v, &b);
        m_volume[i]->setValue(v);
        m_balance[i]->setValue(b);
        addChild(m_volume[i], 0, (i*4)+1, 20, 1);
        addChild(m_balance[i], 0, (i*4)+3, 20, 1);
    }
    m_button = WSYSObject::create<Button>("Close");

    addChild(m_button, 0, 17, 20, 1);

}

void AudioWidget::drawImpl()
{
    screenptr_t ptr = m_screenptr;
    ptr+=2;
    ptr.drawstring("Beeper volume:");
    ptr.nextcharline();
    ptr.nextcharline();
    ptr.drawstring("Beeper balance:");
    ptr.nextcharline();
    ptr.nextcharline();
    ptr.drawstring("AY Ch0 volume:");
    ptr.nextcharline();
    ptr.nextcharline();
    ptr.drawstring("AY Ch0 balance:");
    ptr.nextcharline();
    ptr.nextcharline();
    ptr.drawstring("AY Ch1 volume:");
    ptr.nextcharline();
    ptr.nextcharline();
    ptr.drawstring("AY Ch1 balance:");
    ptr.nextcharline();
    ptr.nextcharline();
    ptr.drawstring("AY Ch2 volume:");
    ptr.nextcharline();
    ptr.nextcharline();
    ptr.drawstring("AY Ch2 balance:");
}

