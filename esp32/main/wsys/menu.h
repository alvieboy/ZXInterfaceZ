#include "widget.h"
#include <stdlib.h>
#include <vector>

#define MENU_COLOR_NORMAL			0x78
#define MENU_COLOR_DISABLED			0x38
#define MENU_COLOR_SELECTED			0x68

#define MENU_FLAGS_DISABLED (1<<0)

struct MenuEntry {
    uint8_t flags;
    const char *string;
};

struct MenuEntryList {
    const MenuEntry &operator[](int idx) const {
        ESP_LOGI("WSYS", "Req index %d max %d %p", idx, sz, &entries[idx]);
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
};

