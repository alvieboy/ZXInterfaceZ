#pragma once

#include "menu.h"

class CallbackMenu: public Menu
{
public:
    CallbackMenu(Widget *parent=NULL): Menu(parent) {}

    typedef void (*Function)(void);

    void setCallbackTable(const Function f[]);

protected:
    virtual void activateEntry(uint8_t entry) override;

    const Function *m_functions;
    void *m_userdata;
};

