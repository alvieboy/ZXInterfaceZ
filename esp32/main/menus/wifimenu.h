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

class WifiMode: public VLayout
{
public:
    WifiMode(Widget *parent=NULL): VLayout(parent)
    {
        m_text = new WifiModeText();
        m_button = new Button(NULL,"Change mode");
        addChild(m_text, LAYOUT_FLAG_VEXPAND);
        addChild(m_button);
    }
private:
    WifiModeText *m_text;
    Button *m_button;
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

