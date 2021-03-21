#include "slider.h"
#include "window.h"
#include "audiowindow.h"
#include "audio.h"
#include "fixedlayout.h"
#include "button.h"
#include "spectrum_kbd.h"

class AudioWidget: public FixedLayout
{
public:
    AudioWidget();
    virtual void drawImpl() override;
    void apply();
    Signal<> &clicked() { return m_button->clicked(); }
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
    setWindowHelpText("Use CAPS key for fine-tuning");

    m_audiowidget->clicked().connect(this , &AudioWindow::saveAndClose);
    setFocusKeys(KEY_DOWN, KEY_UP);
}

void AudioWindow::saveAndClose()
{
    m_audiowidget->apply();
    destroy();
}

AudioWidget::AudioWidget()
{
    float v, b;
    unsigned i;
#if 0
    const char accels[4][4] = {
        { 'a','q','s','w' },
        { 'd','e','f','r' },
        { 'g','t','h','y' },
        { 'j','u','k','i' },
    };
    const char uaccels[4][4] = {
        { 'A','Q','S','W' },
        { 'D','E','F','R' },
        { 'G','T','H','Y' },
        { 'J','U','K','I' },
    };
#endif
    for (i=0;i<4; i++) {

        m_volume[i] = WSYSObject::create<FloatSlider>();
        m_balance[i] = WSYSObject::create<FloatSlider>();

        m_volume[i]->setMinMax(0.0F, 1.0F);
        //m_volume[i]->setAccelMinor(uaccels[i][0],uaccels[i][1],0.01F);
        //m_volume[i]->setAccelMajor(accels[i][0],accels[i][1],0.1F);
        m_volume[i]->setAccelMinor('o','p',0.01F);
        m_volume[i]->setAccelMajor('O','P',0.1F);

        m_balance[i]->setMinMax(-1.0F, 1.0F);
        m_balance[i]->setAccelMinor('o','p',0.01F);
        m_balance[i]->setAccelMajor('O','P',0.1F);

        audio__get_volume_f(i, &v, &b);
        m_volume[i]->setValue(v);
        m_balance[i]->setValue(b);
        addChild(m_volume[i], 0, (i*4)+1, 20, 1);
        addChild(m_balance[i], 0, (i*4)+3, 20, 1);
    }
    m_button = WSYSObject::create<Button>("Close");

    addChild(m_button, 0, 17, 20, 1);
    m_button->setAccelKey(KEY_ENTER);
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

void AudioWidget::apply()
{
    unsigned i;
    for (i=0;i<4; i++) {
        float volume = m_volume[i]->getValue();
        float balance = m_balance[i]->getValue();
        audio__set_volume_f(i, volume, balance);
    }
}
