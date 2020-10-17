#include "about.h"
#include "vlayout.h"
#include "hlayout.h"
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

AboutWindow::AboutWindow(): Window("About", 32, 18)
{
    m_layout = WSYSObject::create<VLayout>();
    m_buttonlayout = WSYSObject::create<HLayout>();
    m_closebutton =    WSYSObject::create<Button>("Close [ENTER]");
    m_firmwarebutton = WSYSObject::create<Button>("Update [F]");
    m_about =  WSYSObject::create<AboutWidget>();
    m_layout->addChild(m_about, LAYOUT_FLAG_VEXPAND);
    m_buttonlayout->addChild(m_firmwarebutton);
    m_buttonlayout->addChild(m_closebutton);
    m_layout->addChild(m_buttonlayout);
    m_firmwarebutton->setSpacing(1);
    m_firmwarebutton->setAccelKey('f');
    m_closebutton->setSpacing(1);

    m_closebutton->clicked().connect(this, &AboutWindow::closeButtonClicked );
    setChild(m_layout);
}

void AboutWindow::closeButtonClicked()
{
    destroy();
}
