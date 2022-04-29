#pragma once

#include "callbackmenu.h"
#include "window.h"

class MenuWindow: public Window
{
public:
    MenuWindow(const char *title, uint8_t w, uint8_t h);

    void setEntries(const MenuEntryList *entries);

    void setCallbackTable(const CallbackMenu::Function f[]);
    void setHelpStrings(const char *help[]);
protected:

private:
    CallbackMenu *m_menu;
};

