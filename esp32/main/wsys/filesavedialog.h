#include "dialog.h"
#include <string>
#include <vector>
#include "fileentry.h"

class FileListMenu;
class MenuEntryList;
class EditBox;
class FixedLayout;
class Frame;
class FileFilter;
class Label;

class FileSaveDialog: public Dialog
{
public:
    FileSaveDialog(const char*title, uint8_t w, uint8_t h, const FileFilter *filter, uint8_t flags=0);
    virtual ~FileSaveDialog();
    void setFilter(const FileFilter*);
    void setFileName(const char *c);
    virtual int exec() override;
    const char *getSelection() const;
    void onDirectoryChanged(const char*);

protected:
    void activate(uint8_t index);
    FileListMenu *m_menu;
    EditBox *m_editbox;
    FixedLayout *m_layout;
    Frame *m_frame;
    Label *m_filetypelabel;
};
