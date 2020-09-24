#include "menu.h"
#include "../spectrum_kbd.h"

#define DAMAGE_SELECTION DAMAGE_USER1
#define DAMAGE_CONTENTS  DAMAGE_USER2

Menu::Menu(Widget *parent): Widget(parent)
{
    m_displayOffset = 0;
    m_selectedEntry = 0;
    m_entries = NULL;
}

void Menu::handleEvent(uint8_t type, u16_8_t code)
{
    if (type!=0)
        return;

    char c = spectrum_kbd__to_ascii(code.v);
    
    ESP_LOGI("WSYS", "Menu event kbd 0x%02x", c);

    switch (c) {
    case 'a':
        chooseNext();
        break;
    case 'q':
        choosePrev();
        break;
    case KEY_ENTER:
        if (!(m_entries->entries[m_selectedEntry].flags & MENU_FLAGS_DISABLED))
            activateEntry( m_selectedEntry );
        break;
    case KEY_BREAK:
        activateEntry( 0xff );
        break;
    }
}


void Menu::setEntries(const MenuEntryList *entries)
{
    m_entries = entries;
    damage(DAMAGE_SELECTION|DAMAGE_CONTENTS);
}

void Menu::draw(bool force)
{
    if (force) {
        damage(DAMAGE_SELECTION|DAMAGE_CONTENTS);
    }
    drawImpl();
}

void Menu::drawImpl()
{
    if (damage() & (DAMAGE_SELECTION)) {
        updateSelection();
    }
    if (damage() & (DAMAGE_CONTENTS)) {
        drawContents();
    }
    clear_damage(DAMAGE_SELECTION|DAMAGE_CONTENTS);
}

void Menu::fillSline(attrptr_t attr, uint8_t value)
{
    ESP_LOGI("WSYS","fillsline attr %d", attr.getoff());

    for (int i=0;i<width();i++) {
        *attr++ = value;
    }
}

void Menu::updateSelection()
{
    int pos = 0;
    unsigned int currententry = m_displayOffset;
    attrptr_t attr = m_attrptr;
    uint8_t attrval;

    unsigned int maxentries = m_entries->size();

    ESP_LOGI("WSYS","update %p %d %d (attr %d)", this, pos, height(), attr.getoff());

    ESP_LOGI("WSYS","selected entry %d offset %d", m_selectedEntry, m_displayOffset);
    while (pos < height()) {
        if (currententry < maxentries && currententry==m_selectedEntry) {
            const MenuEntry &ref = (*m_entries)[currententry];
            if (ref.flags & MENU_FLAGS_DISABLED) {
                attrval = MENU_COLOR_DISABLED;
            } else {
                attrval = MENU_COLOR_SELECTED;
            }
        } else {
            attrval = MENU_COLOR_NORMAL;
        }
        fillSline(attr, attrval);
        attr.nextline();
        pos++;
        currententry++;
    };
}


void Menu::drawContents()
{
    unsigned int maxentries = m_entries->size();
    screenptr_t screenptr = m_screenptr;

    unsigned int numlines = height();

    maxentries -= m_displayOffset;

    if (numlines > maxentries)
        numlines = maxentries;

    unsigned int currententry = m_displayOffset;

    ESP_LOGI("WSYS", "Menu: will draw %d lines", numlines);

    //screenptr++;

    while (numlines--) {
        const MenuEntry &ref = (*m_entries)[currententry];
        drawItem( screenptr, ref.string );
        screenptr.nextcharline();
        currententry++;
    }
}

void Menu::drawItem(screenptr_t screenptr, const char *what)
{
    screenptr.drawstringpad(what, width());
}

void Menu::chooseNext()
{
    uint8_t sel = m_selectedEntry;
    sel++;
    if (sel>=m_entries->size())
        return;
    ESP_LOGI("WSYS", "Menu select next");
    m_selectedEntry = sel;

    if (sel > (m_displayOffset+height())) {
        m_displayOffset++;
        damage(DAMAGE_CONTENTS);
    }
    damage(DAMAGE_SELECTION);
}

void Menu::choosePrev()
{
    if (m_selectedEntry==0)
        return;
    m_selectedEntry--;
    if (m_selectedEntry<m_displayOffset) {
        m_displayOffset--;
        damage(DAMAGE_CONTENTS);
    }
    damage(DAMAGE_SELECTION);
}

