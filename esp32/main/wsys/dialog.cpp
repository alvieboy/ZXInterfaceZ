#include "dialog.h"
#include "screen.h"

Dialog::Dialog(const char *title, uint8_t w, uint8_t h): Window(title,w,h)
{
}

void Dialog::setResult(int val)
{
    m_result=val;
    //m_callback(m_cbdata, m_result);
    // close
    //screen__removeWindow(this);
    setVisible(false);
}

int Dialog::exec()
{
 //   m_cbdata = data;
  //  m_callback = callback;
    WSYS_LOGI( "Displaying dialog!");
    screen__addWindowCentered(this);
    setVisible(true);
    screen__windowLoop(this);
    return m_result;
}

