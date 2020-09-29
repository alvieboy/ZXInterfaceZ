#include "vbar.h"
#include "charmap.h"

void VBar::drawImpl()
{
    int i;
    WSYS_LOGI("Redraw");
    screenptr_t ptr = m_screenptr;

    for (i=0;i<height();i++) {
        ptr.drawchar(CENTERVERTICAL);
        ptr.nextcharline();
    }
}
