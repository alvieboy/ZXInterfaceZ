#include "dialog.h"
#include "screen.h"

Dialog::Dialog(const char *title, uint8_t w, uint8_t h): Window(title,w,h)
{
    m_result = -1;
    setVisible(false);
    screen__addWindowCentered(this);
}

void Dialog::setResult(int val)
{
    m_result=val;
    setVisible(false);
}

int Dialog::exec()
{
 //   m_cbdata = data;
  //  m_callback = callback;
    WSYS_LOGI( "Displaying dialog!");
//    screen__addWindowCentered(this);
    setVisible(true);
    screen__windowLoop(this);
    return m_result;
}


void Dialog::accept(int retval)
{
    WSYS_LOGI("Accepting dialog %d", retval);
    setResult(retval);
}

void Dialog::reject()
{
    setResult(-1);
}
