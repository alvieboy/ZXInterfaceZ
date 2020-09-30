#include "wifimenu.h"
#include "object_signal.h"

static const MenuEntryList wifimenu_entries = {
    .sz = 4,
    .entries = {
        { .flags = 0, .string = "Status" },
        { .flags = 0, .string = "Mode" },
        { .flags = 0, .string = "Wireless" },
        { .flags = 0, .string = "Network" },
        { .flags = 0, .string = "Advanced" },
    }
};


WifiMenu::WifiMenu(): Window("Wifi settings", 32, 20)
{
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
   // m_menu->setFunctionHandler( [](void*arg,uint8_t index){ static_cast<WifiMenu*>(arg)->selected(index);} ,this);

    m_menu->selectionChanged().connect( this, &WifiMenu::selected ) ;
}

void WifiMenu::selected(uint8_t index)
{
    WSYS_LOGI("SELECTED %d this=%p", index, this);
    m_stack->setCurrentIndex(index);
}
