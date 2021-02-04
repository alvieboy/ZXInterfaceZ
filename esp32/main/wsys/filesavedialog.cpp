#include "filesavedialog.h"
#include <cstring>
#include "../fileaccess.h"
#include <algorithm>
#include "filelistmenu.h"
#include "screen.h"
#include "editbox.h"
#include "fixedlayout.h"
#include "frame.h"

FileSaveDialog::FileSaveDialog(const char*title, uint8_t w, uint8_t h, uint8_t flags): Dialog(title, w,h)
{
    m_menu = create<FileListMenu>();
    m_editbox = create<EditBox>();
    m_layout = create<FixedLayout>();
    m_frame = create<Frame>("Navigator", w-2, h-6);

    setChild(m_layout);

    m_editbox->setEditable(true);
    m_layout->addChild(m_editbox, 0, 0, w-2, 2);

    m_frame->setChild(m_menu);
    m_layout->addChild(m_frame, 0, 3, w-2, h-6);
    m_menu->selected().connect( this, &FileSaveDialog::activate );
}

void FileSaveDialog::activate(uint8_t index)
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

int FileSaveDialog::exec()
{
    if (!m_menu->buildDirectoryList()) {
        WSYS_LOGE("Cannot build directory list!");
        return -1;
    }

    WSYS_LOGI( "Loaded directory list");
    return Dialog::exec();
}

FileSaveDialog::~FileSaveDialog()
{
    WSYS_LOGI("Destroying FileSaveDialog");
    //releaseResources();
}

const char *FileSaveDialog::getSelection() const
{
    return m_menu->getSelection();
}

void FileSaveDialog::setFilter(uint8_t filter)
{
    m_menu->setFilter(filter);
    //m_filter = filter;
}
