#include "window.h"
#include <vector>
#include "screen.h"
extern "C" {
#include "esp_log.h"
};

static std::vector<Window*> windows;
typedef std::vector<Window*>::const_iterator window_iter_t;

void screen__init()
{
    //windows.clear();
}

void screen__destroyAll()
{
    windows.clear();
}



void screen__addWindow(Window*win, uint8_t x, uint8_t y)
{
    ESP_LOGI("WSYS", "Adding screen window");
    windows.push_back(win);
    win->move(x, y);
    win->damage(DAMAGE_WINDOW);
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
    ESP_LOGI("WSYS", "Redraw windows %d", windows.size());
    for (auto i: windows) {
        ESP_LOGI("WSYS", "Redraw window %p", i);
        i->draw(true);
    }
    wsys__send_to_fpga();
}

void screen__keyboard_event(u16_8_t k)
{
    if (k.l==0xff)
        return;
    unsigned l = windows.size();
    ESP_LOGI("WSYS", "KBD event");
    windows[l-1]->handleEvent(0,k);

    //wsys__send_to_fpga();
}

static Window * screen__getActiveWindow()
{
    unsigned l = windows.size();
    if (l==0)
        return NULL;
    return windows[l-1];
}

static Window * screen__getWindow(int index)
{
    if (index<0)
        return NULL;
    return windows[index];
}

static inline bool screen__checkEnclosed(Widget *source, Widget *w)
{
    if (source->x() < w->x()) {
        return false;
    }

    if (source->y() > w->y()) {
        return false;
    }

    if (source->x2() > w->x2()) {
        return false;
    }

    if (source->y2() < w->y2()) {
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
        wsys__get_screen_from_fpga();
    }

    ESP_LOGI("WSYS", "Force window redraw");

    while (w != windows.end()) {
        (*w)->draw(true);
        (*w)->clear_damage();
        w++;
    }
    wsys__send_to_fpga();
}

void screen__removeWindow(Window*w)
{
    windows.pop_back();
    screen__damage(w);
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
        ESP_LOGI("WSYS", "Check_Redraw: Redrawing window");
        win->draw();
        ESP_LOGI("WSYS", "Updating spectrum image");
        wsys__send_to_fpga();
    }


}

