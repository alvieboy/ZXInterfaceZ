#include "indexedmenu.h"
#include "window.h"

class MenuWindowIndexed: public Window
{
public:
    MenuWindowIndexed(const char *title, uint8_t w, uint8_t h);

    void setEntries(const MenuEntryList *entries);

    void setCallbackFunction(const IndexedMenu::Function f, void *data);
    void setHelpStrings(const char *help[]);
protected:

private:
    IndexedMenu *m_menu;
};

