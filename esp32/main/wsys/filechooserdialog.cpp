#include "filechooserdialog.h"
#include <cstring>
#include "../fileaccess.h"
#include <algorithm>
#include "indexedmenu.h"
#include "screen.h"

FileChooserDialog::FileChooserDialog(const char*title, uint8_t w, uint8_t h): Dialog(title, w,h)
{
    m_menu = create<IndexedMenu>();
    setChild(m_menu);
    m_menu->selected().connect( this, &FileChooserDialog::activate );
    m_menudata = NULL;
    m_menulistdata = NULL;
    m_filter = FILE_FILTER_NONE;
}

static bool filter_match(uint8_t filter, struct dirent *e)
{
    const char *ext;
    if (e->d_type == DT_DIR) {
        return true;
    } else {
        if (filter==FILE_FILTER_NONE)
            return true;

        ext = get_file_extension(e->d_name);
        /* THIS IS DUPLICATED. Move into separate module */
        switch(filter) {
        case FILE_FILTER_SNAPSHOTS:
            return ext_match(ext, "sna") | ext_match(ext,"z80");
            break;
        case FILE_FILTER_ROMS:
            return ext_match(ext, "rom");
            break;
        case FILE_FILTER_TAPES:
            return ext_match(ext, "tap") || ext_match(ext,"tzx");
            break;
        default:
            break;
        }
    }
    return false;
}


static bool filecompare(const FileEntry &a, const FileEntry &b)
{
    //int i;
    const char *pa = a.str();
    const char *pb = b.str();

    WSYS_LOGI( "CMP %p %p", &a, &b);
    WSYS_LOGI( "CMPPA %s %s", pa, pb);

    uint8_t type_a = a.flags();
    uint8_t type_b = b.flags();

    if (type_a>type_b)
        return true; // Directories before files

    return strcmp(pa, pb)<0;
}

bool FileChooserDialog::buildMountpointList()
{
    FileEntryList l_entries;

    const struct mountpoints *mp = __get_mountpoints();
    int i;

    for (i=0;i<mp->count;i++) {
        FileEntry entry(1, mp->mounts[i]);
        l_entries.push_back(entry);
    }

    releaseResources();

    m_menulistdata = Menu::allocEntryList(l_entries.begin(), l_entries.end(), m_menudata);
    m_menu->setEntries( m_menulistdata );

    return true;
}



bool FileChooserDialog::buildDirectoryList()
{
    char cwd[128];
    FileEntryList l_entries;

    struct dirent *ent;
    //int entries = 0;
    //unsigned char *bptr;
    //unsigned alloc_size;

    DIR *dir;

    if (__in_rootdir()) {
        return buildMountpointList();
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
    m_menu->setEntries( m_menulistdata );
    return true;
}

void FileChooserDialog::releaseResources()
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

void FileChooserDialog::activate(uint8_t index)
{
    WSYS_LOGI("Activate index %d",index);
    if (index==0xff) {
        // Closed, abort.
        releaseResources();
        //screen__removeWindow(this);
        setResult(-1);
        return;
    }
    const MenuEntry *e = m_menu->getEntry(index);
    WSYS_LOGI("Flags %d, '%s'",e->flags, e->string);
    uint8_t fileflags = e->flags >> 1; //: tbd this should be in menu
    if (fileflags == 1) {
        // Directory
        __chdir(e->string);
        releaseResources();
        buildDirectoryList();
    } else {
        setResult(0);
    }
}

int FileChooserDialog::exec()
{
    buildDirectoryList();
    WSYS_LOGI( "Loaded directory list");
    return Dialog::exec();
}

FileChooserDialog::~FileChooserDialog()
{
    WSYS_LOGI("Destroying FileChooserDialog");
    releaseResources();
}

const char *FileChooserDialog::getSelection() const
{
    return m_menu->getEntry()->string;
}

void FileChooserDialog::setFilter(uint8_t filter)
{
    m_filter = filter;
}
