#include "filesavedialog.h"
#include <cstring>
#include "../fileaccess.h"
#include <algorithm>
#include "filelistmenu.h"
#include "screen.h"
#include "editbox.h"
#include "fixedlayout.h"
#include "frame.h"
#include "label.h"
#include "button.h"
#include "hlayout.h"

FileSaveDialog::FileSaveDialog(const char*title, uint8_t w, uint8_t h,
                               const FileFilter *filter,
                               uint8_t flags): Dialog(title, w,h)
{
    m_frame = create<Frame>("Browsing", w-2, h-10);
    m_menu = create<FileListMenu>();
    m_menu->setFilter(filter);

    m_editbox = create<EditBox>();
    // TBD: convert this to VLayout once we fix the layouts...
    m_layout = create<FixedLayout>();

    m_menu->directoryChanged().connect(this, &FileSaveDialog::onDirectoryChanged);

    setChild(m_layout);

    m_editbox->setEditable(true);
    m_layout->addChild( create<Label>("Name: "), 0, 0, w-2, 1);

    m_layout->addChild(m_editbox, 6, 0, w-2-6, 1);

    m_layout->addChild(m_frame, 0, 3, w-2, h-10);


    m_menu->selected().connect( this, &FileSaveDialog::activate );

    Label *label1 = create<Label>("Type:");
    m_layout->addChild( label1, 0+6, 1, 5, 1);

    m_filetypelabel  = create<Label>(filter->getDescription());
    m_layout->addChild( m_filetypelabel, 5+1+6, 1, w-2-11-6, 1);


    HLayout *button_layout = create<HLayout>();

    Button *button1 = create<Button>("Save");
    Button *button2 = create<Button>("Cancel");

    button1->clicked().connect( [this]{ this->accept(0); });
    button2->clicked().connect( static_cast<Dialog*>(this), &Dialog::reject);

    //m_layout->addChild( button1, 2, h-6, 8, 1);
    //m_layout->addChild( button2, 18, h-6, 8, 1);
    button_layout->setSpacing(1);
    button_layout->addChild(button1, LAYOUT_FLAG_HEXPAND);
    button_layout->addChild(button2, LAYOUT_FLAG_HEXPAND);
    m_layout->addChild( button_layout, 0, h-6, w-2, 1);

    m_frame->setChild(m_menu);

}

void FileSaveDialog::onDirectoryChanged(const char *c)
{
    char temp[512];
    if (c==NULL)
        c="/";
    sprintf(temp,"Browsing %s", c);
    m_frame->setTitle(temp);
}

void FileSaveDialog::activate(uint8_t index)
{
    char file[128];
    WSYS_LOGI("Activate index %d",index);
    if (index==0xff) {
        // Closed, abort.
        //releaseResources();
        //screen__removeWindow(this);
        //setResult(-1);
        return;
    }
    // Extract selection from input.
    const MenuEntry *entry = m_menu->getEntry();
    if (!entry)
        return;

    const char *name = entry->string;

    // Extract extension from filename
    const char *ext = get_file_extension(name);
    if (!ext) {
        // No extension??!?!
        return;
    }
    unsigned namesize = (ext-name)-1;

    strlcpy(file, name, namesize);
    file[namesize] = '\0';

    WSYS_LOGI("File name is: '%s'", file);
    m_editbox->setText(file);
    //setResult(0);
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
    return m_editbox->getText();
}

void FileSaveDialog::setFilter(const FileFilter *filter)
{
    m_menu->setFilter(filter);
    //m_filter = filter;
}
void FileSaveDialog::setFileName(const char *c)
{
    m_editbox->setText(c);
}
