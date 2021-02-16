#include "layout.h"
#include <climits>

Layout::Layout(Widget *parent): MultiWidget(parent)
{
    m_spacing = 0;
    m_border = 0;
}

void Layout::addChild(Widget *w, uint8_t flags) {
    m_flags.push_back(flags);
    MultiWidget::addChild(w);
}

void Layout::drawImpl()
{
}

void Layout::setSpacing(uint8_t s)
{
    m_spacing = s;
    redraw();
}

static int shortest(const std::vector<uint8_t> &flags, uint8_t flag, std::vector<int> &sizes)
{
    unsigned index=0;
    int found_index=-1;
    int min_size = INT_MAX;

    for (index=0; index<sizes.size();index++) {
        if (!(flags[index] & flag))
            continue;
        if (sizes[index]<min_size) {
            min_size = sizes[index];
            found_index = index;
        }
    }
    return found_index;
}

void Layout::computeLayout(std::vector<int> &sizes,
                           uint8_t (Widget::*sizefun)(void) const,
                           uint8_t flag,
                           int size)
{
    if (getNumberOfChildren()<1)
        return;

    sizes.resize(getNumberOfChildren());

    int avail = size - (m_border*2) - (getNumberOfChildren()-1)*m_spacing;

    WSYS_LOGI("Avail: %d\n", avail);
    int min = 0;
    int index=0;
    for (auto i: childs()) {
        if (!i->isVisible())
            continue;
        int thisminsize = (i->*sizefun)();
        min+=thisminsize;
        sizes[index++] = thisminsize;
    }
    WSYS_LOGI("Min: %d\n", min);
    WSYS_LOGI("Avail to expand: %d\n", avail-min);
    int toexpand = avail-min;
    while (toexpand>0) {
        int item = shortest(m_flags, flag, sizes);
        if (item<0)
            break;
        WSYS_LOGI("Expanding item %d (size %d) toexpand=%d\n", item, sizes[item], toexpand);
        sizes[item]++;
        toexpand--;
    }
    index=0;

    int pos = m_border;

    for (auto i: childs()) {
        if (!i->isVisible())
            continue;
        WSYS_LOGI("Item %d: size %d -> pos %d to %d\n", index, sizes[index], pos, pos+sizes[index]-1);
        pos+=sizes[index];
        pos+=m_spacing;
        index++;
    }
}

void Layout::visibilityOrFocusPolicyChanged(Widget *w)
{
    if (w!=this) {
        resizeEvent();
    }
    if (m_parent)
        m_parent->visibilityOrFocusPolicyChanged(w);
}
