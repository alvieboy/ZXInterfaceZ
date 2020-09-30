#include "window.h"
#include "hlayout.h"
#include "indexedmenu.h"
#include "label.h"
#include "vbar.h"
#include "stackedwidget.h"

class WifiMenu: public Window
{
public:
    WifiMenu();
    void selected(uint8_t index);
private:
    HLayout *m_hl;
    IndexedMenu *m_menu;
    Label *m_test;
    StackedWidget *m_stack;
};
