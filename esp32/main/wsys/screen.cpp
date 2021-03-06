#include "window.h"
#include <vector>
#include "screen.h"
extern "C" {
#include "esp_log.h"
};
#include <algorithm>

static std::vector<Window*> windows;
static std::vector<Window*> window_cleanup;

typedef std::vector<Window*>::const_iterator window_iter_t;
static Widget *kbdfocuswidget = NULL;

static Window * screen__getActiveWindow();

void screen__init()
{
}

void screen__add_to_cleanup(Window *s)
{
    if (std::find(window_cleanup.begin(), window_cleanup.end(), s)==window_cleanup.end())
        window_cleanup.push_back(s);
}

void screen__do_cleanup()
{
    while (window_cleanup.size()) {
        Window *w = window_cleanup.back();
        window_cleanup.pop_back();
        WSYS_LOGI("Deleting %p (%s)\n", w, CLASSNAME(*w));
        delete(w);
    }
}

void screen__destroyAll()
{
    WSYS_LOGI( "Destroying all windows (%d)", windows.size());
    for (auto i:windows) {
        screen__add_to_cleanup(i);
    }
    windows.clear();
    screen__damage(NULL);
}



void screen__addWindow(Window*win, uint8_t x, uint8_t y)
{
    WSYS_LOGI("Adding screen window");
    WSYS_LOGI("Windows %d", windows.size());

    Window *current =screen__getActiveWindow();
    if (current)
        current->focusOut();

    windows.push_back(win);
    win->move(x, y);
    if (win->isVisible()) {
        win->setdamage(DAMAGE_WINDOW);
        win->show();
    }
}

void screen__windowVisibilityChanged(Window *win, bool visible)
{
    WSYS_LOGI("Window visibility changed %p %d\n", win, visible);
    if (visible) {
        win->resizeEvent();
        win->setdamage(DAMAGE_WINDOW);
    }
}



void screen__addWindowCentered(Window*win)
{
    int x = win->width();
    x>>=1;
    x = 16 - x;
    int y = 12 - (win->height()>>1);
    screen__addWindow(win, x, y);
}

void screen__redraw()
{
    WSYS_LOGI( "Redraw windows %d", windows.size());
    for (auto i: windows) {
        WSYS_LOGI( "Redraw window %p", i);
        i->draw(true);
    }
    wsys__send_to_fpga();
}

static void screen__event(wsys_input_event_t evt)
{
    unsigned l = windows.size();

    if (l==0)
        return;

    if (kbdfocuswidget) {
        WSYS_LOGI( "KBD event (grabbed)");
        kbdfocuswidget->handleEvent(evt);
    } else {
        WSYS_LOGI( "KBD event (window %d)", l-1);
        windows[l-1]->handleEvent(evt);
    }

}

void screen__keyboard_event(u16_8_t k)
{
    wsys_input_event_t evt;

    if (k.l==0xff)
        return;

    evt.type = WSYS_INPUT_EVENT_KBD;
    evt.code = k;
    screen__event(evt);
}

void screen__joystick_event(joy_action_t action, bool on)
{
    wsys_input_event_t evt;
    evt.type = WSYS_INPUT_EVENT_JOYSTICK;
    evt.joy_on = on;
    evt.joy_action = action;
    screen__event(evt);
}



static Window * screen__getActiveWindow()
{
    unsigned l = windows.size();
    if (l==0)
        return NULL;
    return windows[l-1];
}


static inline bool screen__checkEnclosed(Widget *source, Widget *w)
{
    WSYS_LOGI( "Check enclosed (%d,%d,%d,%d) (%d,%d,%d,%d)",
             source->x(),
             source->y(),
             source->x2(),
             source->y2(),
             w->x(),
             w->y(),
             w->x2(),
             w->y2());

    if (source->x() < w->x()) {
        return false;
    }

    if (source->y() < w->y()) {
        return false;
    }

    if (source->x2() > w->x2()) {
        return false;
    }

    if (source->y2() > w->y2()) {
        return false;
    }
    return true;
}

void screen__damage(Widget *source)
{
    int index = windows.size() - 1;
    bool enclosed = false;
    bool redraw_root = false;

    window_iter_t w = windows.end();

    WSYS_LOGI("Damage: checking windows (%d)", index);

    if (!windows.size()) {
        redraw_root = true;
    } else {

        w = windows.begin() + index;

        do {
            enclosed = screen__checkEnclosed(source, *w);
            if (!enclosed) {
                index--;
                if (index<0) {
                    redraw_root = true;
                    break;
                } else {
                    w--;
                }
            } else {
                break;
            }
        } while (!enclosed);

    }
    if (redraw_root) {
        //
        WSYS_LOGI( "Force root redraw");
        wsys__get_screen_from_fpga();
    }

    WSYS_LOGI( "Force window redraw (index %d)", index);

    while (w != windows.end()) {
        (*w)->draw(true);
        (*w)->clear_damage();
        w++;
    }
    wsys__send_to_fpga();
}

void screen__removeWindow(Window*w)
{
    std::vector<Window*>::iterator i = std::find(windows.begin(), windows.end(), w);
    if (i!=windows.end()) {
        WSYS_LOGI("Removing window %p %s", w, CLASSNAME(*w));
        WSYS_LOGI("Windows %d", windows.size());
        windows.erase(i);
        WSYS_LOGI("Windows now %d", windows.size());
        //windows.pop_back();


        w->focusOut(); // Do we need this ?
        screen__damage(w);
        screen__add_to_cleanup(w);
        Window *current = screen__getActiveWindow();
        if (current) {
            current->focusIn();
        }
    } else {
        WSYS_LOGW("Removing already destroyed window!!!");
//        abort();
    }
}



void screen__loop(Widget *d)
{
    while (d->visible()) {
        wsys__eventloop_iter();
    }
}

void screen__check_redraw()
{
    Window *win = screen__getActiveWindow();
    if (win && win->needRedraw()) {
        WSYS_LOGI( "Check_Redraw: Redrawing window");
        win->draw();
        WSYS_LOGI( "Updating spectrum image");
        wsys__send_to_fpga();
    }
}

void screen__grabKeyboardFocus(Widget *d)
{
    if (kbdfocuswidget) {
        WSYS_LOGE("ERROR: trying to grab already-grabbed keyboard focus!!!");
    }
    WSYS_LOGI("Grabbing keyboard %p",d);
    kbdfocuswidget = d;
}

void screen__releaseKeyboardFocus(Widget *d)
{
    if (kbdfocuswidget==d)
        kbdfocuswidget=NULL;
}

void screen__windowLoop(Window *w)
{
    do {
        if (std::find(windows.begin(), windows.end(), w)!=windows.end()) {
            if (w->visible()) {
                wsys__eventloop_iter();
            } else {
                break;
            }
        } else {
            break;
        }
    } while (1);
}
