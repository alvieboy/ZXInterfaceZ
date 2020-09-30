#ifndef __WSYS_MENUWINDOWINDEXED_H__
#define __WSYS_MENUWINDOWINDEXED_H__

#include "indexedmenu.h"
#include "window.h"

class MenuWindowIndexed: public Window
{
public:
    MenuWindowIndexed(const char *title, uint8_t w, uint8_t h);

    void setEntries(const MenuEntryList *entries);
    void setHelpStrings(const char *help[]);
    Signal<uint8_t> &selected() { return m_menu->selected(); }

private:
    IndexedMenu *m_menu;
};

#endif
