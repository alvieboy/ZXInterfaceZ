#ifndef __WSYS_FILELISTMENU_H__
#define __WSYS_FILELISTMENU_H__

#include "indexedmenu.h"

class FileListMenu: public IndexedMenu
{
public:
    FileListMenu();
    virtual ~FileListMenu();
    bool buildDirectoryList();
    void setFilter(uint8_t filter);
    const char *getSelection() const;

protected:
    void activate(uint8_t index);
    bool buildMountpointList();
    void releaseResources();
    virtual void activateEntry(uint8_t entry);

private:
    void *m_menudata;
    MenuEntryList *m_menulistdata;

    std::string m_cwd;
    uint8_t m_filter;

};

#endif
