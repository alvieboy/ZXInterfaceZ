#include "menuwindowindexed.h"

MenuWindowIndexed::MenuWindowIndexed(const char *title, uint8_t w, uint8_t h): Window(title, w, h)
{
    m_menu = new IndexedMenu();
    setChild(m_menu);
}

void MenuWindowIndexed::setEntries(const MenuEntryList *entries)
{
    m_menu->setEntries(entries);
}

void MenuWindowIndexed::setHelpStrings(const char *help[])
{
    m_menu->setHelp(help, this);
}

void MenuWindowIndexed::setActiveEntry(uint8_t entry)
{
    m_menu->setActiveEntry(entry);
}
