#include "window.h"
#include "hlayout.h"
#include "indexedmenu.h"
#include "label.h"
#include "vbar.h"
#include "stackedwidget.h"
#include "vlayout.h"
#include "button.h"

class WifiStatus: public Widget
{
public:
    WifiStatus(Widget *parent=NULL): Widget(parent) { }

    virtual void handleEvent(uint8_t type, u16_8_t code) {}
    virtual void drawImpl();
};

class WifiModeText: public Widget
{
public:
    WifiModeText(Widget *parent=NULL): Widget(parent) { }

    virtual void handleEvent(uint8_t type, u16_8_t code) {}
    virtual void drawImpl();
};


static const MenuEntryList wifimodeselectormenu = {
    .sz = 3,
    .entries = {
        { .flags = 0, .string = "Access Point mode" },
        { .flags = 0, .string = "Station mode" },
        { .flags = 0, .string = "Back" }
    }
};

class WifiMode: public VLayout
{
public:
    WifiMode(Widget *parent=NULL): VLayout(parent)
    {
        m_text = new WifiModeText();
        m_button = new Button(NULL,"Change mode");
        addChild(m_text, LAYOUT_FLAG_VEXPAND);
        addChild(m_button);
        m_button->clicked().connect( this, &WifiMode::changeMode );
    }
    void changeMode() {
        m_modeselwindow = new MenuWindowIndexed("WiFi mode", 18, 6);
        m_modeselwindow->selected().connect( this, &WifiMode::modeSelected );
        screen__addWindowCentered(m_modeselwindow);
        m_modeselwindow->show();
    }
private:
    WifiModeText *m_text;
    Button *m_button;
    MenuWindowIndexed *m_modeselwindow;
};

class WifiMenu: public Window
{
public:
    WifiMenu();
    void selected(uint8_t index);
    void activated(uint8_t index);
private:
    HLayout *m_hl;
    IndexedMenu *m_menu;
    WifiStatus *m_status;
    WifiMode *m_mode;
    StackedWidget *m_stack;
};

