#include "os/queue.h"
#include "os/task.h"
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
#include "savemenu.h"
#include "esp_timer.h"
#include "filesavedialog.h"
#include "standardfilefilter.h"
#include "joystick.h"
#include "struct_assert.h"

struct wsys_event {
    uint8_t type;
    union {
        u16_8_t data;
        struct {
            joy_action_t joy_action;
            bool joy_on;
        };
        systemevent_t sysevent;
    };
};


static wsys_mode_t wsys_mode = WSYS_MODE_NMI;

static Queue wsys_evt_queue = NULL;
volatile bool can_update = false;

static void wsys_systemevent_handleevent(const systemevent_t *event, void*user);
static void (*memoryreadcompletecallback)(void*,uint8_t);
static void *memoryreadcompletecallbackdata;

void wsys__keyboard_event(uint16_t raw, char ascii)
{
    struct wsys_event evt;
    evt.data.v = raw;
    evt.type = EVENT_KBD;
    queue__send(wsys_evt_queue, &evt, OS_MAX_DELAY);
    task__yield();
}

void wsys__joystick_event(joy_action_t action, bool on)
{
    struct wsys_event evt;
    evt.joy_action = action;
    evt.joy_on = on;
    evt.type = EVENT_JOYSTICK;
    queue__send(wsys_evt_queue, &evt, OS_MAX_DELAY);
    task__yield();
}

void wsys__memoryreadcomplete(uint8_t len)
{
    struct wsys_event evt;

    WSYS_LOGI("Memory read complete %d\n", len);

    evt.data.v = len;
    evt.type = EVENT_MEMORYREADCOMPLETE;
    queue__send(wsys_evt_queue, &evt, OS_MAX_DELAY);
    task__yield();
}

void wsys__get_screen_from_fpga()
{
    fpga__read_extram_block(MEMLAYOUT_NMI_SCREENAREA, &spectrum_framebuffer.w32[0], sizeof(spectrum_framebuffer));
}

void wsys__nmiready()
{
    can_update = true;
    struct wsys_event evt;
    evt.type = EVENT_NMIENTER;
    queue__send(wsys_evt_queue, &evt, OS_MAX_DELAY);
    task__yield();
}

void wsys__nmileave()
{
    can_update = false;
    struct wsys_event evt;
    evt.type = EVENT_NMILEAVE;
    queue__send(wsys_evt_queue, &evt, OS_MAX_DELAY);
}


static void wsys__start(wsys_mode_t mode)
{
    wsys__get_screen_from_fpga();
    WSYS_LOGI("Starting WSYS mode %d\n", mode);
    switch (mode) {
    case WSYS_MODE_LOAD:
        loadmenu__show();
        break;
    case WSYS_MODE_SAVE:
        savemenu__show();
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
    queue__send(wsys_evt_queue, &evt, OS_MAX_DELAY);
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
    case EVENT_JOYSTICK:
        screen__joystick_event(evt.joy_action, evt.joy_on);
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
    case EVENT_MEMORYREADCOMPLETE:
        if (memoryreadcompletecallback) {
            memoryreadcompletecallback(memoryreadcompletecallbackdata, evt.data.v);
        }
        break;
    }
}

void wsys__eventloop_iter()
{
    struct wsys_event evt;
    WSYS_LOGI("Wait for event");
    if (queue__receive(wsys_evt_queue, &evt, OS_MAX_DELAY)==OS_TRUE) {
        WSYS_LOGI("Dispatch event");
        wsys__dispatchevent(evt);
    }
    screen__check_redraw();
}

void wsys__task(void *data __attribute__((unused)))
{
    WSYS_LOGI("Starting task");
    screen__init();
    //queue__send(wsys_evt_queue, &gpio_num, NULL);
    while (1) {
        wsys__eventloop_iter();
        screen__do_cleanup();
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
    wsys_evt_queue = queue__create(4, sizeof(struct wsys_event));
    if (task__create(wsys__task, "wsys_task", 4096, NULL, 9, NULL)!=OS_TRUE) {
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

    systemevent__register_handler(0xFF, wsys_systemevent_handleevent, NULL);
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


static void wsys_systemevent_handleevent(const systemevent_t *event, void*user)
{
    struct wsys_event evt;
    evt.type = EVENT_SYSTEM;
    evt.sysevent = *event; // Check copy!
    queue__send(wsys_evt_queue, &evt, OS_MAX_DELAY);
}


static void wsys__requestmemromread(uint16_t address, uint8_t size, void(*callback)(void *,uint8_t),void*user, uint8_t cmd)
{
    uint8_t info[3];
    info[0] = size;
    info[1] = address & 0xff;
    info[2] = address >>8;
    ESP_LOGI("DEBUG", "Requesting mem read address %04x len %d cmd %02x\n", address, size, cmd);
    fpga__write_extram_block(0x021B01, &info[0], sizeof(info));
    memoryreadcompletecallback = callback;
    memoryreadcompletecallbackdata = user;
    wsys__send_command(cmd);
}

void wsys__requestromread(uint16_t address, uint8_t size, void(*callback)(void *,uint8_t),void*user)
{
    wsys__requestmemromread(address, size, callback, user, 0xFA);
}

void wsys__requestmemread(uint16_t address, uint8_t size, void(*callback)(void *,uint8_t),void*user)
{
    wsys__requestmemromread(address, size, callback, user, 0xFB);
}
