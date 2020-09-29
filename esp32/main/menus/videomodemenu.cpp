#include "videomodemenu.h"
#include "wsys.h"
#include "wsys/menuwindowindexed.h"
#include "wsys/screen.h"
#include "vga.h"

static MenuWindowIndexed *videomode_window;

static void videomode__set(void *data, uint8_t index)
{
    switch (index) {
    case 3:
    case 0xff:
        screen__removeWindow(static_cast<Window*>(data));
        break;
    default:
        vga__setmode(index);
        break;
    }
}

static const MenuEntryList videomodemenu = {
    .sz = 4,
    .entries = {
        { .flags = 0, .string = "Expanded display" },
        { .flags = 0, .string = "Wide-screen display" },
        { .flags = 0, .string = "Normal display" },
        { .flags = 0, .string = "Back" },
    }
};

void videomodemenu__show()
{
    videomode_window = new MenuWindowIndexed("Video mode", 22, 7);
    videomode_window->setEntries( &videomodemenu );
    videomode_window->setCallbackFunction( &videomode__set, videomode_window);
    screen__addWindowCentered(videomode_window);
    videomode_window->setVisible(true);
}

