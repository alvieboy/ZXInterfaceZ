#include "filechooserdialog.h"
#include <cstring>
#include "../fileaccess.h"
#include <algorithm>
#include "filelistmenu.h"
#include "screen.h"

FileChooserDialog::FileChooserDialog(const char*title, uint8_t w, uint8_t h, const FileFilter *filter): Dialog(title, w,h)
{
    m_menu = create<FileListMenu>(filter);
    setChild(m_menu);
    m_menu->selected().connect( this, &FileChooserDialog::activate );
}

void FileChooserDialog::activate(uint8_t index)
{
    WSYS_LOGI("Activate index %d",index);
    if (index==0xff) {
        // Closed, abort.
        //releaseResources();
        //screen__removeWindow(this);
        setResult(-1);
        return;
    }
    setResult(0);
}

int FileChooserDialog::exec()
{
    if (!m_menu->buildDirectoryList()) {
        WSYS_LOGE("Cannot build directory list!");
        return -1;
    }

    WSYS_LOGI( "Loaded directory list");
    return Dialog::exec();
}

FileChooserDialog::~FileChooserDialog()
{
    WSYS_LOGI("Destroying FileChooserDialog");
    //releaseResources();
}

const char *FileChooserDialog::getSelection() const
{
    return m_menu->getSelection();
}

void FileChooserDialog::setFilter(const FileFilter *filter)
{
    m_menu->setFilter(filter);
    //m_filter = filter;
}
