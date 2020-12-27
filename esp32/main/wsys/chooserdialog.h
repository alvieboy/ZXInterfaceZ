#include "dialog.h"
#include <string>
#include <vector>

class IndexedMenu;
class MenuEntryList;

struct Entry {
    Entry(const char *str, void*user): m_name(str), m_user(user) {}
    Entry() {};
    void *user() const { return m_user; }
    std::string m_name;
    void *m_user;
    const char *str() const {return m_name.c_str();}
    int flags() { return 0; }
};

typedef std::vector<Entry> EntryList;

class ChooserDialog: public Dialog
{
public:
    ChooserDialog(const char*title, uint8_t w, uint8_t h);
    virtual ~ChooserDialog();

    void setEntries(EntryList &list);

    virtual int exec() override;
    const char *getSelection() const;
protected:
    virtual void activate(uint8_t index);
    void releaseResources();
    IndexedMenu *m_menu;
    void *m_menudata;
    MenuEntryList *m_menulistdata;
};
