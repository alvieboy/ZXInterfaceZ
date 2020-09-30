#include "indexedmenu.h"

void IndexedMenu::activateEntry(uint8_t entry)
{
    m_selected.emit(entry);
}

