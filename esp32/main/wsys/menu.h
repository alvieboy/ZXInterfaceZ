#ifndef __WSYS_MENU_H__
#define __WSYS_MENU_H__

#include "widget.h"
#include <stdlib.h>
#include <vector>


#define MENU_COLOR_NORMAL			0x78
#define MENU_COLOR_DISABLED			0x38
#define MENU_COLOR_SELECTED			0x68

#define MENU_FLAGS_DISABLED (1<<0)

class HelpDisplayer;

struct MenuEntry {
    uint8_t flags;
    const char *string;
};

struct MenuEntryList {
    const MenuEntry &operator[](int idx) const {
        return entries[idx];
    }
    int size() const { return sz; }
    int sz;
    MenuEntry entries[];
};

class Menu: public Widget
{
public:
    Menu(Widget *parent);
    virtual void handleEvent(uint8_t type, u16_8_t code) override;
    void setEntries( const MenuEntryList *);
    void draw(bool force=false) override;
    void setHelp(const char *helpstrings[], HelpDisplayer *displayer);
    template<class Iter>
        static MenuEntryList *allocEntryList(Iter first, Iter last, void *&data) {
            int count = std::distance(first,last);
            uint8_t *menudatabuf;
            char *cdata;
            // First, allocate full list.
            menudatabuf = (uint8_t*)malloc( sizeof(MenuEntryList) + count * sizeof(MenuEntry));

            if (!menudatabuf) {
                ESP_LOGE("WSYS", "Menu: cannot allocate");
                return NULL;
            }

            MenuEntryList *l = (MenuEntryList*)menudatabuf;

            l->sz = count;

            int idx = 0;

            // Do a first pass to compute string sizes.
            unsigned strsize = 0;
            for (Iter s = first; s!=last; s++) {
                strsize += strlen(s->str()) + 1;
            }

            // Allocate strings

            cdata = (char*)malloc(strsize);

            data = cdata;

            if (cdata==NULL) {
                ESP_LOGE("WSYS", "Menu: cannot allocate cdata");
                free(menudatabuf);
                return NULL;
            }


            idx = 0;
            for (Iter s=first; s!=last; s++, idx++)
            {
                char *start = cdata;
                cdata = stpcpy(cdata, s->str());
                cdata++;
                l->entries[idx].string = start;
                l->entries[idx].flags = s->flags() << 1;  // first bit reserved
            }

            return l;
        }
    const MenuEntry *getEntry(int index) {
        return &m_entries->entries[index];
    }
    const MenuEntry *getEntry() {
        return &m_entries->entries[m_selectedEntry];
    }

    virtual uint8_t getMinimumWidth() const;

protected:
    void chooseNext();
    void choosePrev();
    void drawContents();
    void updateSelection();

    void fillSline(attrptr_t attr, uint8_t value);

    virtual void drawItem(screenptr_t screenptr, const char *what);

    
    virtual void drawImpl() override;
    virtual void activateEntry(uint8_t entry) = 0;
    uint8_t m_displayOffset;
    uint8_t m_selectedEntry;
    //    std::vector<MenuEntry>* m_entries;
    const MenuEntryList *m_entries;
    HelpDisplayer *m_helpdisplayer;
    const char **m_helpstrings;
};



#endif
