#include "videomodemenu.h"
#include "wsys.h"
#include "wsys/menuwindowindexed.h"
#include "wsys/screen.h"
#include "vga.h"

static VideoModeWindow *videomode_window;

void VideoModeWindow::set(uint8_t index)
    {
        switch (index) {
        case 3:
        case 0xff:
            screen__removeWindow(this);
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

VideoModeWindow::VideoModeWindow(): MenuWindowIndexed("Video mode", 22, 7)
{
    setEntries( &videomodemenu );
    selected().connect(this, &VideoModeWindow::set );
}

void videomodemenu__show()
{
    videomode_window = new VideoModeWindow();
    screen__addWindowCentered(videomode_window);
    videomode_window->setVisible(true);
}

