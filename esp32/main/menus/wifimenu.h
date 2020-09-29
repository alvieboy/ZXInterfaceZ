#include "window.h"
#include "hlayout.h"
#include "indexedmenu.h"
#include "label.h"
#include "vbar.h"
#include "stackedwidget.h"

static const MenuEntryList wifimenu_entries = {
    .sz = 4,
    .entries = {
        { .flags = 0, .string = "Status" },
        { .flags = 0, .string = "DHCP" },
        { .flags = 0, .string = "DHCP1" },
        { .flags = 0, .string = "DHCP2" },
    }
};


class WifiMenu: public Window
{
public:
    WifiMenu(): Window("Wifi settings", 28, 20) {
        m_hl = new HLayout();
        setChild(m_hl);
        m_menu = new IndexedMenu();
        m_hl->addChild(m_menu);

        m_hl->addChild( new VBar() );

        m_stack = new StackedWidget();

        m_test = new Label("Help");

        m_stack->addChild(m_test);
        m_stack->addChild(new Label("Opt1"));
        m_stack->addChild(new Label("Opt2"));
        m_stack->addChild(new Label("Opt3"));


        m_hl->addChild(m_stack, LAYOUT_FLAG_HEXPAND);
        m_menu->setEntries(&wifimenu_entries);
        m_menu->setFunctionHandler( [](void*arg,uint8_t index){ static_cast<WifiMenu*>(arg)->selected(index);} ,this);
    }

    void selected(uint8_t index)
    {
        m_stack->setCurrentIndex(index);
    }
private:
    HLayout *m_hl;
    IndexedMenu *m_menu;
    Label *m_test;
    StackedWidget *m_stack;
};
