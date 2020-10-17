#ifndef __ABOUT_H__
#define __ABOUT_H__

#include "window.h"

class AboutWidget;
class VLayout;
class HLayout;
class Button;

class AboutWindow: public Window
{
public:
    AboutWindow();
    void closeButtonClicked();
    void firmwareButtonClicked();
private:
    AboutWidget *m_about;
    VLayout *m_layout;
    HLayout *m_buttonlayout;;
    Button *m_closebutton;
    Button *m_firmwarebutton;
};

#endif
