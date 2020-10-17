#include "about.h"
#include "vlayout.h"
#include "button.h"
#include "version.h"
#include "fpga.h"

class AboutWidget: public Widget
{
public:
    AboutWidget();
    virtual void drawImpl();
};


AboutWidget::AboutWidget()
{
    redraw();
}

void AboutWidget::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    screenptr_t t;
    parentDrawImpl();

    screenptr.drawstring("ZX Spectrum information");
    screenptr.nextcharline();
    t = screenptr;
    t++;
    t.printf("Model: %s", get_spectrum_model());
    screenptr.nextcharline();
    screenptr.nextcharline();
    screenptr.drawstring("Interface Z information");
    screenptr.nextcharline();
    screenptr.nextcharline();
    screenptr.drawstringpad(version, 28);
    screenptr.nextcharline();
    screenptr.drawstringpad(gitversion, 28);
    screenptr.nextcharline();
    screenptr.drawstringpad(builddate, 28);
    screenptr.nextcharline();
    t = screenptr;
    t = t.drawstring("FPGA version: ");

    unsigned id = fpga__read_id();

    t.printf("%08x", id);
}

AboutWindow::AboutWindow(): Window("About", 30, 18)
{
    m_layout = WSYSObject::create<VLayout>();
    m_button = WSYSObject::create<Button>("Close [ENTER]");
    m_about =  WSYSObject::create<AboutWidget>();
    m_layout->addChild(m_about, LAYOUT_FLAG_VEXPAND);
    m_layout->addChild(m_button);
    m_button->clicked().connect(this, &AboutWindow::buttonClicked );
    setChild(m_layout);
}

void AboutWindow::buttonClicked()
{
    destroy();
}
