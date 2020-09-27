#include "dialog.h"
#include <string>
#include <vector>

class IndexedMenu;
class MenuEntryList;

struct FileEntry {
public:
    FileEntry(): m_name("") {}
    FileEntry(uint8_t flags, const char *str): m_flags(flags), m_name(str) {}
    uint8_t m_flags;
    std::string m_name;
    const char *str() const {return m_name.c_str(); }
    uint8_t flags() const { return m_flags; }
};

typedef std::vector<FileEntry> FileEntryList;

class FileChooserDialog: public Dialog
{
public:
    FileChooserDialog(const char*title, uint8_t w, uint8_t h);
    virtual ~FileChooserDialog();
    bool buildDirectoryList();
    void setFilter(uint8_t filter);
    virtual void exec( void (*callback)(void*, int), void*) override;
    const char *getSelection() const;
protected:
    void activate(uint8_t index);
    bool buildMountpointList();
    void releaseResources();
    IndexedMenu *m_menu;
    void *m_menudata;
    MenuEntryList *m_menulistdata;

    std::string m_cwd;
    uint8_t m_filter;

};
