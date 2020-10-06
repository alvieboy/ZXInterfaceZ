#ifndef __WSYS_DIALOG_H__
#define __WSYS_DIALOG_H__

#include "window.h"

class Dialog: public Window
{
public:
    Dialog(const char *title, uint8_t w, uint8_t h);

    void setResult(int val);
    int result() const { return m_result; }
    virtual int exec();//void (*callback)(void*, int), void*);
private:
    int m_result;
    //void (*m_callback)(void*, int);
    //void *m_cbdata;

};

#endif
