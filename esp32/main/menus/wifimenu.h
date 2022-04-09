#pragma once

#include "window.h"
#include "hlayout.h"
#include "indexedmenu.h"
#include "label.h"
#include "vbar.h"
#include "stackedwidget.h"
#include "vlayout.h"
#include "button.h"
#include "menuwindowindexed.h"
#include "screen.h"

class WifiStatus: public Widget
{
public:
    WifiStatus(Widget *parent=NULL): Widget(parent) { }

    //virtual bool handleEvent(wsys_input_event_t evt) override { return false; }
    virtual void drawImpl() override;
};

class WifiModeText: public Widget
{
public:
    WifiModeText(Widget *parent=NULL): Widget(parent) { }

    //virtual bool handleEvent(uint8_t type, u16_8_t code) override { return false; }
    virtual void drawImpl() override;
};


class WifiMode: public VLayout
{
public:
    WifiMode(Widget *parent=NULL);
    void changeMode();
    void modeSelected(uint8_t val);
    Signal<> &modechanged() { return m_modechanged; }
    void systemEvent(const systemevent_t &event);
private:
    WifiModeText *m_text;
    Button *m_button;
    MenuWindowIndexed *m_modeselwindow;
    Signal<> m_modechanged;
    int systemnotifyslot;
};

class WifiWirelessSettings;

class WifiMenu: public Window
{
public:
    WifiMenu();
    void selected(uint8_t index);
    void activated(uint8_t index);
    void modechanged();
private:
    HLayout *m_hl;
    IndexedMenu *m_menu;
    WifiStatus *m_status;
    WifiMode *m_mode;
    WifiWirelessSettings *m_wifisettings;
    StackedWidget *m_stack;
};


