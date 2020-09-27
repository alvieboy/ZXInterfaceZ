#include "dialog.h"
#include "screen.h"

Dialog::Dialog(const char *title, uint8_t w, uint8_t h): Window(title,w,h)
{
}

void Dialog::setResult(uint8_t val) {
    m_result=val;
    m_callback(m_cbdata, m_result);
    // close
    screen__removeWindow(this);
}

void Dialog::exec( void (*callback)(void*, int), void*data)
{
    m_cbdata = data;
    m_callback = callback;
    WSYS_LOGI( "Displaying dialog!");
    screen__addWindowCentered(this);
    setVisible(true);
}

