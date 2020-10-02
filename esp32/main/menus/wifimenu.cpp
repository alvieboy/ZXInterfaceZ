#include "wifimenu.h"
#include "object_signal.h"
#include "wifi.h"
#include "screen.h"
static const MenuEntryList wifimenu_entries = {
    .sz = 6,
    .entries = {
        { .flags = 1, .string = "Status" },
        { .flags = 1, .string = "Mode" },
        { .flags = 1, .string = "Wireless" },
        { .flags = 1, .string = "Network" },
        { .flags = 1, .string = "Advanced" },
        { .flags = 0, .string = "Back" },
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

    m_status = new WifiStatus();
    m_mode   = new WifiMode();

    m_stack->addChild(m_status);
    m_stack->addChild(m_mode);
    m_stack->addChild(new Label("Opt2"));
    m_stack->addChild(new Label("Opt3"));
    m_stack->addChild(new Label("Opt4"));
    m_stack->addChild(new Label(""));


    m_hl->addChild(m_stack, LAYOUT_FLAG_HEXPAND);
    m_menu->setEntries(&wifimenu_entries);
   // m_menu->setFunctionHandler( [](void*arg,uint8_t index){ static_cast<WifiMenu*>(arg)->selected(index);} ,this);

    m_menu->selectionChanged().connect( this, &WifiMenu::selected ) ;
    m_menu->selected().connect( this, &WifiMenu::activated ) ;
}

void WifiMenu::selected(uint8_t index)
{
    m_stack->setCurrentIndex(index);
}

void WifiMenu::activated(uint8_t index)
{
    switch(index) {
    case 5:
        screen__removeWindow(this);
        break;
    }
}


/* ********************************************************************************************** */

void WifiStatus::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    screenptr_t temp;
    uint32_t addr;
    uint32_t netmask;
    uint32_t gw;

    parentDrawImpl();

    screenptr.drawstring("WiFi status: ");


    screenptr.nextcharline();
    temp = screenptr;
    screenptr++;

    if (wifi__issta()) {
        screenptr.drawstring("STA");
        screenptr = temp;
        screenptr.nextcharline();
    } else {
        screenptr.drawstring("Access Point mode");

        screenptr = temp;
        screenptr.nextcharline();
        screenptr.nextcharline();

        screenptr.printf("Clients: %d", wifi__get_clients());
        screenptr.nextcharline();
    }

    screenptr.nextcharline();
    screenptr.drawstring("IP configuration:");
    screenptr.nextcharline();
    screenptr.nextcharline();

    int r = wifi__get_ip_info(&addr,&netmask,&gw);
    if (r==0) {
        screenptr.printf("IP  : %d.%d.%d.%d",
                         (addr>>24)&0xff,
                         (addr>>16)&0xff,
                         (addr>>8)&0xff,
                         (addr&0xff));

        screenptr.nextcharline();

        screenptr.printf("Mask: %d.%d.%d.%d",
                         (netmask>>24)&0xff,
                         (netmask>>16)&0xff,
                         (netmask>>8)&0xff,
                         (netmask&0xff));

        screenptr.nextcharline();

        screenptr.printf("GW  : %d.%d.%d.%d",
                         (gw>>24)&0xff,
                         (gw>>16)&0xff,
                         (gw>>8)&0xff,
                         (gw&0xff));

    } else {
        screenptr.drawstring("<no information>");
    }

}

/* ********************************************************************************************** */

void WifiModeText::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    screenptr_t temp = screenptr;

    parentDrawImpl();

    screenptr.drawstring("Current mode:");

    screenptr.nextcharline();
    temp = screenptr;
    screenptr++;

    if (wifi__issta()) {
        screenptr.drawstring("STA");
    } else {
        screenptr.drawstring("Access Point mode");
    }
    screenptr = temp;
    screenptr.nextcharline();
    screenptr.nextcharline();
    screenptr.drawstring("Press ENTER to ");
    screenptr.nextcharline();
    screenptr.drawstring("change mode");
}
