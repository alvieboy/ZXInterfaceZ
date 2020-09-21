#include "callbackmenu.h"

void CallbackMenu::setCallbackTable(const CallbackMenu::Function f[])
{
    m_functions = f;
}

void CallbackMenu::activateEntry(uint8_t entry)
{
    m_functions[entry]();
}

