#pragma once

#include "indexedmenu.h"
#include "standardfilefilter.h"
#include "systemevent.h"

class FileListMenu: public IndexedMenu
{
public:
    FileListMenu(const FileFilter *filter=StandardFileFilter::AllFilesFileFilter());
    virtual ~FileListMenu();
    bool buildDirectoryList();
    void setFilter(const FileFilter*);
    const char *getSelection() const;
    Signal<const char *> &directoryChanged() { return m_directoryChanged; }

protected:
    void activate(uint8_t index);
    bool buildMountpointList();
    void releaseResources();
    virtual void activateEntry(uint8_t entry) override;
    static void systemEventHandler(const systemevent_t *event, void *user);
    void systemEventHandler(const systemevent_t *event);
private:
    void *m_menudata;
    systemevent_handlerid_t m_handler;
    MenuEntryList *m_menulistdata;
    Signal<const char *> m_directoryChanged;
    std::string m_cwd;
    const FileFilter *m_filter;

};
