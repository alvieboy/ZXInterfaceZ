#pragma once

#include "window.h"
#include "slider.h"

class AudioWidget;

class AudioWindow: public Window
{
public:
    AudioWindow();
protected:
    void saveAndClose();
private:
    AudioWidget *m_audiowidget;
};
