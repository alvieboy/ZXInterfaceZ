#include "fileaccess.h"
#include "filelistmenu.h"
#include "fileentry.h"
#include <algorithm>

FileListMenu::FileListMenu(const FileFilter *filter)
{
    m_menudata = NULL;
    m_menulistdata = NULL;
    m_filter = filter;

    m_handler =
        systemevent__register_handler(SYSTEMEVENT_TYPE_STORAGE,
                                      &FileListMenu::systemEventHandler,
                                      this);
}

void FileListMenu::systemEventHandler(const systemevent_t *event, void *user)
{
    static_cast<FileListMenu*>(user)->systemEventHandler(event);
}

void FileListMenu::systemEventHandler(const systemevent_t *event)
{
    if (__in_rootdir()) {
        buildDirectoryList();
        redraw();
    }
}


FileListMenu::~FileListMenu()
{
    systemevent__unregister_handler(m_handler);
    releaseResources();
}


static bool filter_match(const FileFilter *filter, struct dirent *e)
{
    const char *ext;
    if (e->d_type == DT_DIR) {
        return true;
    } else {
        ext = get_file_extension(e->d_name);
        return filter->match_extension(ext);
    }
    return false;
}

static bool filecompare(const FileEntry &a, const FileEntry &b)
{
    //int i;
    const char *pa = a.str();
    const char *pb = b.str();

    uint8_t type_a = a.flags();
    uint8_t type_b = b.flags();

    if (type_a>type_b)
        return true; // Directories before files

    return strcmp(pa, pb)<0;
}

bool FileListMenu::buildMountpointList()
{
    FileEntryList l_entries;

    const struct mountpoints *mp = __get_mountpoints();
    int i;
    WSYS_LOGI("Mountpoints in system: %d\n", mp->count);
    if (mp->count==0) {
        WSYS_LOGI("No mountpoints to show!");
        return false;
    }

    for (i=0;i<mp->count;i++) {
        FileEntry entry(1, mp->mounts[i]);
        l_entries.push_back(entry);
    }

    releaseResources();

    m_menulistdata = Menu::allocEntryList(l_entries.begin(), l_entries.end(), m_menudata);
    setEntries( m_menulistdata );

    return true;
}



bool FileListMenu::buildDirectoryList()
{
    char cwd[128];
    FileEntryList l_entries;

    struct dirent *ent;
    //int entries = 0;
    //unsigned char *bptr;
    //unsigned alloc_size;

    DIR *dir;

    if (__in_rootdir()) {
        bool r = buildMountpointList();
        if (r)
            m_directoryChanged.emit("/");
        return r;
    }

    if (__getcwd(cwd,sizeof(cwd))==NULL) {
        WSYS_LOGE("Cannot get root directory!");
        return false;
    }

    WSYS_LOGI("Loading directory '%s'", cwd);


    // TODO: get cwd short name

    char *cdir = strrchr(cwd,'/');
    if (NULL==cdir) {
        WSYS_LOGE("Cannot extract current dir from '%s'", cwd);
        return false;
    }
    if (cdir[1] != '\0') {
        cdir++;
    }

    WSYS_LOGI("Current visible dir: '%s'", cdir);
    m_cwd = cdir;



    l_entries.clear();

    FileEntry rootdir(1,"..");

    l_entries.push_back(rootdir);

    // 1st pass: get name lengths
    dir = __opendir(cwd);
    if (NULL==dir) {
        cwd[0] = '/';
        cwd[1] = '\0';
        cdir = cwd;
        dir = __opendir(cwd);
        if (!dir) {
            WSYS_LOGI("Cannot open dir: '%s'", strerror(errno));
            return false;
        }
    }
    while ((ent=__readdir(dir))) {
        if (filter_match(m_filter, ent)) {
            uint8_t flags;
            if (ent->d_type==DT_DIR) {
                flags = 1;
            } else {
                flags = 0;
            }
            FileEntry entry(flags, ent->d_name);
            l_entries.push_back(entry);
        }
    }

    closedir(dir);

    std::sort( l_entries.begin(), l_entries.end(), filecompare);

    releaseResources();

    m_menulistdata = Menu::allocEntryList(l_entries.begin(), l_entries.end(), m_menudata);
    setEntries( m_menulistdata );

    m_directoryChanged.emit(cwd);

    return true;
}

void FileListMenu::releaseResources()
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


void FileListMenu::activateEntry(uint8_t index)
{
    WSYS_LOGI("Activate index %d",index);
    if (index==0xff) {
        m_selected.emit(index);
        return;
    }
    const MenuEntry *e = getEntry(index);

    uint8_t fileflags = e->flags >> 1; //: tbd this should be in menu
    if (fileflags == 1) {
        // Directory
        if (__chdir(e->string)==0) {
        }
        releaseResources();
        buildDirectoryList();
    } else {
        m_selected.emit(index);
    }
}

const char *FileListMenu::getSelection() const
{
    return getEntry()->string;
}

void FileListMenu::setFilter(const FileFilter *filter)
{
    m_filter = filter;
}
