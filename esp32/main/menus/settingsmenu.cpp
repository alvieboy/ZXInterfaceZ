#include "settingsmenu.h"
#include "wsys.h"
#include "wsys/menuwindow.h"
#include "wsys/screen.h"
#include "videomodemenu.h"
#include "wifimenu.h"

static MenuWindow *settings_window;

static const MenuEntryList settingsmenu = {
    .sz = 5,
    .entries = {
        { .flags = 0, .string = "Wifi..." },
        { .flags = 1, .string = "Bluetooth..." },
        { .flags = 0, .string = "USB..." },
        { .flags = 0, .string = "Video..." },
        { .flags = 0, .string = "Back" }
    }
};

static void settings__wifi();
static void settings__usb();
static void settings__video();
static void settings__back();

static const CallbackMenu::Function settings_functions[] =
{
    &settings__wifi,
    NULL,
    &settings__usb,
    &settings__video,
    &settings__back
};


void settings__show()
{
    settings_window = new MenuWindow("Settings", 20, 8);
    settings_window->setEntries( &settingsmenu );
    settings_window->setCallbackTable( settings_functions );
    screen__addWindowCentered(settings_window);
    settings_window->setVisible(true);
}

static void settings__wifi()
{
    WifiMenu *wifimenu = new WifiMenu();
    screen__addWindowCentered(wifimenu);
    wifimenu->setVisible(true);
}

static void settings__usb()
{
}
static void settings__video()
{
    videomodemenu__show();
}
static void settings__back()
{
    delete(settings_window);
}
