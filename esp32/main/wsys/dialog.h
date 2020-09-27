#ifndef __WSYS_DIALOG_H__
#define __WSYS_DIALOG_H__

#include "window.h"

class Dialog: public Window
{
public:
    Dialog(const char *title, uint8_t w, uint8_t h);

    void setResult(uint8_t val);

    virtual void exec( void (*callback)(void*, int), void*);
private:
    uint8_t m_result;
    void (*m_callback)(void*, int);
    void *m_cbdata;

};

#endif
