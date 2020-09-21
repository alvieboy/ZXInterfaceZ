#include "indexedmenu.h"

void IndexedMenu::setFunctionHandler(IndexedMenu::Function f, void *userdata)
{
    m_function = f;
    m_userdata = userdata;
}

void IndexedMenu::activateEntry(uint8_t entry)
{
    m_function(m_userdata, entry);
}

