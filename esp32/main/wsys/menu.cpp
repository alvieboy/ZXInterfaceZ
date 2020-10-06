#include "menu.h"
#include "../spectrum_kbd.h"
#include "helpdisplayer.h"
#include <cstring>

#define DAMAGE_SELECTION DAMAGE_USER1
#define DAMAGE_CONTENTS  DAMAGE_USER2

Menu::Menu(Widget *parent): Widget(parent)
{
    m_displayOffset = 0;
    m_selectedEntry = 0;
    m_entries = NULL;
    m_helpdisplayer = NULL;
}
     /*
void Menu::setHelp(const char *helpstrings[], HelpDisplayer *displayer)
{
    m_helpdisplayer = displayer;
    m_helpstrings = helpstrings;

    if (m_helpdisplayer)
        m_helpdisplayer->displayHelpText(m_helpstrings[m_selectedEntry]);
}
       */

void Menu::setHelp(const char *helpstrings[], HelpDisplayer *displayer)
{
    setHelp( [=](uint8_t index){ return helpstrings[index]; }, displayer );
}

void Menu::setHelp(std::function<const char *(uint8_t)> fun, HelpDisplayer *displayer)
{
    m_helpfun = fun;
    m_helpdisplayer = displayer;

    if (m_helpdisplayer)
        m_helpdisplayer->displayHelpText( m_helpfun(m_selectedEntry) );
}

bool Menu::handleEvent(uint8_t type, u16_8_t code)
{
    if (type!=0)
        return false;

    char c = spectrum_kbd__to_ascii(code.v);
    
    WSYS_LOGI( "Menu event kbd 0x%02x", c);
    bool handled = true;

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
    case KEY_BREAK:  /* fall-through */
    case ' ':
        activateEntry( 0xff );
        break;
    default:
        handled = false;
    }
    return handled;
}


void Menu::setEntries(const MenuEntryList *entries)
{
    m_entries = entries;
    m_selectedEntry = 0;
    m_displayOffset = 0;
    setdamage(DAMAGE_SELECTION|DAMAGE_CONTENTS);
}

void Menu::draw(bool force)
{
    if (force) {
        setdamage(DAMAGE_SELECTION|DAMAGE_CONTENTS);
    }
    drawImpl();
}

void Menu::drawImpl()
{
    if (damage() & (DAMAGE_SELECTION)) {
        updateSelection();
    }
    if (damage() & (DAMAGE_CONTENTS)) {
        WSYS_LOGI("Menu redraw contents");
        drawContents();
    }
    clear_damage(DAMAGE_SELECTION|DAMAGE_CONTENTS);
}

void Menu::setActiveEntry(uint8_t entry)
{
    m_selectedEntry=entry;

    if (m_selectedEntry>=height()) {
        m_displayOffset = (m_selectedEntry-height())+1;
    }
    setdamage(DAMAGE_SELECTION|DAMAGE_CONTENTS);
}

void Menu::fillSline(attrptr_t attr, uint8_t value)
{
    WSYS_LOGI("fillsline attr %d", attr.getoff());

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

    WSYS_LOGI("update %p %d %d (attr %d)", this, pos, height(), attr.getoff());

    WSYS_LOGI("selected entry %d offset %d", m_selectedEntry, m_displayOffset);
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
    unsigned int linesdrawn = 0;
    maxentries -= m_displayOffset;

    if (numlines > maxentries)
        numlines = maxentries;

    unsigned int currententry = m_displayOffset;

    WSYS_LOGI( "Menu: will draw %d lines", numlines);

    //screenptr++;

    while (numlines--) {
        const MenuEntry &ref = (*m_entries)[currententry];
        drawItem( screenptr, ref.string );
        screenptr.nextcharline();
        currententry++;
        linesdrawn++;
    }

    if (linesdrawn<height()) {
        clearLines(screenptr, width(), height()-linesdrawn);
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
    WSYS_LOGI( "Menu select next");
    m_selectedEntry = sel;

    m_selectionChanged.emit(m_selectedEntry);

    if (sel >= (m_displayOffset+height())) {
        m_displayOffset++;
        setdamage(DAMAGE_CONTENTS);
    }
    setdamage(DAMAGE_SELECTION);

    if (m_helpdisplayer)
        m_helpdisplayer->displayHelpText( m_helpfun(m_selectedEntry) );
}

void Menu::choosePrev()
{
    if (m_selectedEntry==0)
        return;
    m_selectedEntry--;
    m_selectionChanged.emit(m_selectedEntry);

    if (m_selectedEntry<m_displayOffset) {
        m_displayOffset--;
        setdamage(DAMAGE_CONTENTS);
    }
    setdamage(DAMAGE_SELECTION);

    if (m_helpdisplayer)
        m_helpdisplayer->displayHelpText( m_helpfun(m_selectedEntry) );

}

uint8_t Menu::getMinimumWidth() const
{
    int len = 1;
    if (!m_entries)
        return len;
    for (int i=0;i<m_entries->size();i++) {
        int clen = strlen((*m_entries)[i].string);
        if (clen>len)
            len=clen;
    }
    return len;
}
