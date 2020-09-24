#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "wsys.h"
#include "wsys/screen.h"
#include "wsys/menuwindow.h"
#include "fpga.h"
#include "wsys/messagebox.h"
#include "wsys/pixel.h"

struct wsys_event {
    uint8_t type;
    u16_8_t data;
};

static xQueueHandle wsys_evt_queue = NULL;

static const MenuEntryList mainmenu = {
    .sz = 7,
    .entries = {
        { .flags = 0, .string = "Load snapshot..." },
        { .flags = 0, .string = "Save snapshot..." },
        { .flags = 0, .string = "Play tape..." },
        { .flags = 1, .string = "Poke..." },
        { .flags = 0, .string = "Settings..." },
        { .flags = 0, .string = "Reset" },
        { .flags = 0, .string = "Exit" },
    }
};

void test1(void);

void exit_nmi()
{
    wsys__send_command(0xFF);
}


void test3(void)
{
    MessageBox::show("Help");

}

static const CallbackMenu::Function mainmenu_functions[] =
{
    &test1,
    &test3,
    &test1,
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
    struct wsys_event evt;
    evt.data.v = raw;
    evt.type = 0;
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
}

void wsys__get_screen_from_fpga()
{
    fpga__read_extram_block(0x002006, &spectrum_framebuffer.screen[0], sizeof(spectrum_framebuffer));
}

MenuWindow *mainwindow;

void wsys__nmiready()
{
    struct wsys_event evt;
    evt.type = EVENT_NMIENTER;
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
}

void wsys__nmileave()
{
    struct wsys_event evt;
    evt.type = EVENT_NMILEAVE;
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
}


void wsys__start()
{
    wsys__get_screen_from_fpga();
    mainwindow = new MenuWindow("ZX Interface Z", 24, 10);

    mainwindow->setEntries( &mainmenu );
    mainwindow->setCallbackTable( mainmenu_functions );
    mainwindow->setHelpText("Use Q/A to move, ENTER select");
    screen__addWindowCentered( mainwindow );
    mainwindow->setVisible(true);

    screen__redraw();

}

void wsys__reset()
{
    struct wsys_event evt;
    evt.type = 2;
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
}



static void wsys__dispatchevent(struct wsys_event evt)
{
    switch(evt.type) {
    case EVENT_KBD:
        screen__keyboard_event(evt.data);
        break;
    case EVENT_RESET:
        break;
    case EVENT_NMIENTER:
        //ESP_LOGI("WSYS", "Resetting screen");
        wsys__start();
        break;
    case EVENT_NMILEAVE:
        ESP_LOGI("WSYS", "Reset sequences");
        spectrum_framebuffer.seq = 0;
        wsys__send_command(0x00);
        screen__destroyAll();
        break;
    }
}

void wsys__eventloop_iter()
{
    struct wsys_event evt;
    if (xQueueReceive(wsys_evt_queue, &evt, portMAX_DELAY)) {
        wsys__dispatchevent(evt);
    }
    screen__check_redraw();
}

void wsys__task(void *data __attribute__((unused)))
{
    screen__init();
    //xQueueSend(wsys_evt_queue, &gpio_num, NULL);
    while (1) {
        wsys__eventloop_iter();
    }
}


void wsys__sendEvent()
{
}

void wsys__init()
{
    wsys_evt_queue = xQueueCreate(4, sizeof(struct wsys_event));
    xTaskCreate(wsys__task, "wsys_task", 4096, NULL, 9, NULL);
}


void wsys__send_to_fpga()
{
    spectrum_framebuffer.seq++;
    spectrum_framebuffer.seq &= 0x7F;

    fpga__write_extram_block(0x020000, &spectrum_framebuffer.screen[0], sizeof(spectrum_framebuffer));
}

void wsys__send_command(uint8_t command)
{
    fpga__write_extram_block(0x021B00, &command, 1);
}
