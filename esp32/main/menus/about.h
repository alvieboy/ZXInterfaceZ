#ifndef __ABOUT_H__
#define __ABOUT_H__

#include "window.h"

class AboutWidget;
class VLayout;
class Button;

class AboutWindow: public Window
{
public:
    AboutWindow();
    void buttonClicked();
private:
    AboutWidget *m_about;
    VLayout *m_layout;;
    Button *m_button;
};

#endif
