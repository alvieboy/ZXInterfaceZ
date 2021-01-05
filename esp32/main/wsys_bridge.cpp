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
#include "wsys/filechooserdialog.h"
#include "menus/nmimenu.h"
#include "systemevent.h"
#include "memlayout.h"
#include "loadmenu.h"
#include "esp_timer.h"

struct wsys_event {
    uint8_t type;
    union {
        u16_8_t data;
        systemevent_t sysevent;
    };
};

static wsys_mode_t wsys_mode = WSYS_MODE_NMI;

static xQueueHandle wsys_evt_queue = NULL;
volatile bool can_update = false;

void wsys__keyboard_event(uint16_t raw, char ascii)
{
    struct wsys_event evt;
    evt.data.v = raw;
    evt.type = 0;
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
    portYIELD();
}

void wsys__get_screen_from_fpga()
{
    fpga__read_extram_block(MEMLAYOUT_NMI_SCREENAREA, &spectrum_framebuffer.screen[0], sizeof(spectrum_framebuffer));
}

void wsys__nmiready()
{
    can_update = true;
    struct wsys_event evt;
    evt.type = EVENT_NMIENTER;
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
    portYIELD();
}

void wsys__nmileave()
{
    can_update = false;
    struct wsys_event evt;
    evt.type = EVENT_NMILEAVE;
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
}


static void wsys__start(wsys_mode_t mode)
{
    wsys__get_screen_from_fpga();
    switch (mode) {
    case WSYS_MODE_LOAD:
        loadmenu__show();
        break;
    default:
        nmimenu__show();
        break;
    }
    screen__redraw();
}

void wsys__reset(wsys_mode_t mode)
{
    struct wsys_event evt;
    evt.type = EVENT_RESET;
    evt.data.v = (uint8_t)mode;
    can_update = false;
    wsys__send_command(0x00); // Clear sequence immediatly
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
}

#define MAX_KEYBOARD_TIMER 6

static volatile uint8_t keyboard_timer_cnt = MAX_KEYBOARD_TIMER;

static void wsys__reset_keyboard_timer()
{
    keyboard_timer_cnt = MAX_KEYBOARD_TIMER;
}

static bool wsys__can_propagate_keyboard(uint16_t data)
{
    if (data==0xFFFF)
        return true; // Always process key-release
    if (keyboard_timer_cnt==0)
        return true;
    return false;
}


static void wsys__dispatchevent(struct wsys_event evt)
{
    WSYS_LOGI("Dispatching event %d", evt.type);
    switch(evt.type) {
    case EVENT_KBD:
        if (wsys__can_propagate_keyboard(evt.data)) {
            screen__keyboard_event(evt.data);
        }
        break;
    case EVENT_RESET:
        wsys_mode = (wsys_mode_t)evt.data.v;
        break;
    case EVENT_NMIENTER:
        //WSYS_LOGI( "Resetting screen");
        wsys__start(wsys_mode);
        wsys__reset_keyboard_timer();
        break;
    case EVENT_NMILEAVE:
        WSYS_LOGI( "Reset sequences");
        spectrum_framebuffer.seq = 0;
        wsys__send_command(0x00);
        screen__destroyAll();
        break;
    case EVENT_SYSTEM:
        //
        wsys__propagatesystemevent(evt.sysevent);

        break;
    }
}

void wsys__eventloop_iter()
{
    struct wsys_event evt;
    WSYS_LOGI("Wait for event");
    if (xQueueReceive(wsys_evt_queue, &evt, portMAX_DELAY)) {
        WSYS_LOGI("Dispatch event");
        wsys__dispatchevent(evt);
    }
    screen__check_redraw();
}

void wsys__task(void *data __attribute__((unused)))
{
    WSYS_LOGI("Starting task");
    screen__init();
    //xQueueSend(wsys_evt_queue, &gpio_num, NULL);
    while (1) {
        wsys__eventloop_iter();
    }
}


void wsys__sendEvent()
{
}

static esp_timer_handle_t keyboard_timer_handle;
static void wsys__keyboard_timer(void *data)
{
    if (keyboard_timer_cnt>0)
        keyboard_timer_cnt--;
}

void wsys__init()
{
    wsys_evt_queue = xQueueCreate(4, sizeof(struct wsys_event));
    if (xTaskCreate(wsys__task, "wsys_task", 4096, NULL, 9, NULL)!=pdPASS) {
        ESP_LOGE("WSYS", "Cannot create task");
    }
    WSYS_LOGI("WSYS task created");

    // Create keyboard "debounce" timer.

    const esp_timer_create_args_t keyboard_timer_args = {
        .callback = &wsys__keyboard_timer,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "keyboard",
    };

    ESP_ERROR_CHECK(esp_timer_create(&keyboard_timer_args, &keyboard_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(keyboard_timer_handle, 20000));
}


void wsys__send_to_fpga()
{
    spectrum_framebuffer.seq++;
    spectrum_framebuffer.seq &= 0x7F;
    if (can_update) {
        WSYS_LOGI("Framebuffer update, seq %d\n", spectrum_framebuffer.seq);
        fpga__write_extram_block(0x020000, &spectrum_framebuffer.screen[0], sizeof(spectrum_framebuffer));
    }
}

void wsys__send_command(uint8_t command)
{
    fpga__write_extram_block(0x021B00, &command, 1);
}

void systemevent__handleevent(const systemevent_t *event)
{
    struct wsys_event evt;
    evt.type = EVENT_SYSTEM;
    evt.sysevent = *event; // Check copy!
    xQueueSend(wsys_evt_queue, &evt, portMAX_DELAY);
}

