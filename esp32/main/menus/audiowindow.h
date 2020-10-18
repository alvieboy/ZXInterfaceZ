#include "window.h"
#include "slider.h"

class AudioWidget;

class AudioWindow: public Window
{
public:
    AudioWindow();
private:
    AudioWidget *m_audiowidget;
};
