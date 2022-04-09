#pragma once

#include "menu.h"
#include "object_signal.h"

class IndexedMenu: public Menu
{
public:
    IndexedMenu(Widget *parent=NULL): Menu(parent) {}

    Signal<uint8_t> &selected() { return m_selected; }

protected:
    virtual void activateEntry(uint8_t entry) override;
    Signal<uint8_t> m_selected;
};
