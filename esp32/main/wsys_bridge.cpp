#include "wsys.h"
#include "wsys/screen.h"
#include "wsys/menuwindow.h"
#include "fpga.h"


static const MenuEntryList mainmenu = {
    .sz = 4,
    .entries = {
        { .flags = 0, .string = "Test1" },
        { .flags = 0, .string = "Test2" },
        { .flags = 0, .string = "Test3" },
        { .flags = 0, .string = "Test4" },
    }
};

void test1(void);

void exit_nmi()
{
    wsys__send_command(0xFF);
}

static const CallbackMenu::Function mainmenu_functions[] =
{
    &test1,
    &test1,
    &test1,
    &exit_nmi,
};

static const MenuEntryList settingsmenu = {
    .sz = 5,
    .entries = {
        { .flags = 0, .string = "Wifi..." },
        { .flags = 0, .string = "Bluetooth..." },
        { .flags = 0, .string = "USB..." },
        { .flags = 0, .string = "Video..." },
        { .flags = 0, .string = "Back" }
    }
};

void test2(void);

static const CallbackMenu::Function settings_functions[] =
{
    &test2,
    &test2,
    &test2,
    &test2,
    &test2,
};

static MenuWindow *settings_window;

static void showsettings()
{
    //LD	DE, $1807 ; width=28, height=7
    settings_window = new MenuWindow("Settings", 28, 8);
    settings_window->setEntries( &settingsmenu );
    settings_window->setCallbackTable( settings_functions );
    screen__addWindowCentered(settings_window);
    settings_window->setVisible(true);
}

void test1(void)
{
    ESP_LOGI("WSYS", "Callback");
    showsettings();

}

void test2(void)
{
    delete(settings_window);
}


void wsys__keyboard_event(uint16_t raw, char ascii)
{
    u16_8_t v;
    v.v=raw;
    screen__keyboard_event(v);
}

void wsys__get_screen_from_fpga()
{
    fpga__read_extram_block(0x002006, &spectrum_framebuffer.screen[0], sizeof(spectrum_framebuffer));
}

void wsys__nmiready()
{
    wsys__get_screen_from_fpga();

    MenuWindow *m = new MenuWindow("ZX Interface Z", 24, 8);
    m->setEntries( &mainmenu );
    m->setCallbackTable( mainmenu_functions );
    screen__addWindowCentered(m);
    m->setVisible(true);

    screen__redraw();
}

void wsys__reset()
{
    spectrum_framebuffer.seq = 0;
    screen__destroyAll();
}

void wsys__init()
{
    screen__init();

}

void wsys__send_to_fpga()
{
    spectrum_framebuffer.seq++;
    spectrum_framebuffer.seq &= 0x7F;
    fpga__write_extram_block(0x020000, &spectrum_framebuffer.screen[0], sizeof(spectrum_framebuffer));
}

void wsys__send_command(uint8_t command)
{
    command |= 0x80;
    fpga__write_extram_block(0x021B00, &command, 1);
}
