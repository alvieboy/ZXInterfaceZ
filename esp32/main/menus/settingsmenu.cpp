#include "settingsmenu.h"
#include "wsys.h"
#include "wsys/menuwindow.h"
#include "wsys/screen.h"
#include "videomodemenu.h"
#include "wifimenu.h"
#include "audiowindow.h"

static MenuWindow *settings_window;

static const MenuEntryList settingsmenu = {
    .sz = 6,
    .entries = {
        { .flags = 0, .string = "Wifi..." },
        { .flags = 1, .string = "Bluetooth..." },
        { .flags = 0, .string = "USB..." },
        { .flags = 0, .string = "Video..." },
        { .flags = 0, .string = "Audio..." },
        { .flags = 0, .string = "Back" }
    }
};

static void settings__wifi();
static void settings__usb();
static void settings__video();
static void settings__audio();
static void settings__back();

static const CallbackMenu::Function settings_functions[] =
{
    &settings__wifi,
    NULL,
    &settings__usb,
    &settings__video,
    &settings__audio,
    &settings__back
};


void settings__show()
{
    settings_window = WSYSObject::create<MenuWindow>("Settings", 20, 9);
    settings_window->setEntries( &settingsmenu );
    settings_window->setCallbackTable( settings_functions );
    screen__addWindowCentered(settings_window);
    settings_window->setVisible(true);
}

static void settings__wifi()
{
    WifiMenu *wifimenu = WSYSObject::create<WifiMenu>();
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
    screen__removeWindow(settings_window);
}

static void settings__audio()
{
    WSYS_LOGI("Showitn audio");
    AudioWindow *audio_window = WSYSObject::create<AudioWindow>();
    screen__addWindowCentered(audio_window);
    audio_window->setVisible(true);
}
