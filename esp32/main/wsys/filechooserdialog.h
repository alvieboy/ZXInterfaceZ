#include "dialog.h"
#include <string>
#include <vector>
#include "fileentry.h"

class FileListMenu;
class MenuEntryList;


class FileChooserDialog: public Dialog
{
public:
    FileChooserDialog(const char*title, uint8_t w, uint8_t h);
    virtual ~FileChooserDialog();
    void setFilter(uint8_t filter);
    virtual int exec() override;// void (*callback)(void*, int), void*) override;
    const char *getSelection() const;
protected:
    void activate(uint8_t index);
    FileListMenu *m_menu;
};
