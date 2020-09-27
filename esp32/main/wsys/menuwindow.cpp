#include "menuwindow.h"

MenuWindow::MenuWindow(const char *title, uint8_t w, uint8_t h): Window(title, w, h)
{
    m_menu = new CallbackMenu();
    setChild(m_menu);
}

void MenuWindow::setEntries(const MenuEntryList *entries)
{
    m_menu->setEntries(entries);
}

void MenuWindow::setCallbackTable(const CallbackMenu::Function f[])
{
    m_menu->setCallbackTable(f);
}

void MenuWindow::setHelpStrings(const char *help[])
{
    m_menu->setHelp(help, this);
}
