#include "chooserdialog.h"
#include <cstring>
#include "../fileaccess.h"
#include <algorithm>
#include "indexedmenu.h"
#include "screen.h"

ChooserDialog::ChooserDialog(const char*title, uint8_t w, uint8_t h): Dialog(title, w,h)
{
    m_menu = create<IndexedMenu>();
    setChild(m_menu);
    m_menu->selected().connect( this, &ChooserDialog::activate );
    m_menudata = NULL;
    m_menulistdata = NULL;
}

void ChooserDialog::releaseResources()
{
    if (m_menudata) {
        FREE(m_menudata);
        m_menudata = NULL;
    }
    if (m_menulistdata) {
        FREE(m_menulistdata);
        m_menulistdata = NULL;
    }
}

void ChooserDialog::setEntries(EntryList &l_entries)
{
    m_menulistdata = Menu::allocEntryList(l_entries.begin(), l_entries.end(), m_menudata);
    m_menu->setEntries( m_menulistdata );
}

void ChooserDialog::activate(uint8_t index)
{
    WSYS_LOGI("Activate index %d",index);
    if (index==0xff) {
        // Closed, abort.
        releaseResources();
        //screen__removeWindow(this);
        setResult(-1);
        return;
    }

    //const MenuEntry *e = m_menu->getEntry(index);

    //WSYS_LOGI("Flags %d, '%s'",e->flags, e->string);

    setResult(0);
}

int ChooserDialog::exec()
{
    return Dialog::exec();
}

ChooserDialog::~ChooserDialog()
{
    WSYS_LOGI("Destroying ChooserDialog");
    releaseResources();
}

const char *ChooserDialog::getSelection() const
{
    return m_menu->getEntry()->string;
}

