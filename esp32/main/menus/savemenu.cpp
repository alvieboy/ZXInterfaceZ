#include "wsys.h"
#include "wsys/menuwindow.h"
#include "wsys/screen.h"
#include "wsys/filechooserdialog.h"
#include "wsys/chooserdialog.h"
#include "settingsmenu.h"
#include "fileaccess.h"
#include "fpga.h"
#include "sna.h"
#include "wsys/messagebox.h"
#include "tapeplayer.h"
#include "about.h"
#include "fasttap.h"
#include "inputdialog.h"
#include "poke.h"
#include "nmi_poke.h"
#include "model.h"
#include "save.h"
#include "filesavedialog.h"

static void cb_save_tap();
static void cb_save_tzx();
static void cb_return();

static const CallbackMenu::Function save_functions[] =
{
    &cb_return,
    &cb_save_tap,
    &cb_save_tzx
};

static const MenuEntryList save_entries = {
    .sz = 3,
    .entries = {
        { .flags = 0, .string = "Save to physical tape" },
        { .flags = 0, .string = "Save to TAP" },
        { .flags = 0, .string = "Save to TXZ" }
    }
};

static const char *savemenu_help[] = {
    "Save to standard cassette tape",
    "Save to TAP file",
    "Save to TZX file"
};

void savemenu__show()
{
    WSYS_LOGI("SAVE menu displaying file %s", save__get_requested_name());
    MenuWindow *savemenu = WSYSObject::create<MenuWindow>("SAVE", 26, 15);

    savemenu->setEntries( &save_entries );
    savemenu->setCallbackTable( save_functions );
    savemenu->setWindowHelpText("Use Q/A to move, ENTER select");
    savemenu->setStatusLines(2);
    savemenu->setHelpStrings( savemenu_help );
    screen__addWindowCentered( savemenu );
    savemenu->setVisible(true);
}


#if 0
static int do_save_tape_fast(FileChooserDialog *d, int status)
{
    char fp[128];
    if (status==0) {
        WSYS_LOGI("Tape is: %s", d->getSelection());

        fullpath(d->getSelection(), fp, 127);
        if (fasttap__prepare(fp)==0) {
            screen__destroyAll();
            wsys__send_command(0xFF);
            return 0;
        } else {
            MessageBox::show("Cannot save TZX file");
        }
    }
    return -1;
}

static void do_save_tape(FileChooserDialog *d, int status)
{
    if (status==0) {
        WSYS_LOGI("Tape is: %s", d->getSelection());
        tapeplayer__play(d->getSelection());
        screen__destroyAll();
        wsys__send_command(0xFF);
    }
}

#endif

static int ask_append_overwrite(const char *filename)
{
    MessageBox *confirm = WSYSObject::create<MessageBox>("File exists",30);
    confirm->setText("File already exists\nWhat do you want to do?");
    confirm->addButton("Append",0);
    confirm->addButton("Overwrite",1)->setThumbFont(true);
    confirm->addButton("Cancel",-1);
    return confirm->exec();
}

class FileSaveAppendOverwriteDialog: public FileSaveDialog
{
public:
    FileSaveAppendOverwriteDialog(const char*title, uint8_t w, uint8_t h, const FileFilter *filter, uint8_t flags=0): FileSaveDialog(title, w, h, filter, flags) {}
    virtual void accept(uint8_t val) override;
    const char *getPath() const { return path; }
private:
    char path[128];
};

void FileSaveAppendOverwriteDialog::accept(uint8_t val)
{
    char filename[128];
    bool retry;
    struct stat st;
    bool do_append = false;

    if (val!=0)
        return;

    const char *c = getSelection();

    if (c) {
        strcpy(filename, c);
        if (get_file_extension(filename)==NULL) {
            strcat(filename, ".tap");
        }

        fullpath(filename, path, sizeof(path));

        WSYS_LOGI("Requested file name: %s", path);
        // Check if file exists.
        if (__lstat(path, &st)==0) {
            // Ask whether to append/overwrite or cancel.
            switch (ask_append_overwrite(filename)) {
            case 0: // Append
                do_append = true;
            case 1: // Overwrite
                break;
            default: // Cancel
                return;
            }
            FileSaveDialog::accept( do_append ? 1: 0);

        } else {
            WSYS_LOGI("File does NOT exist");
            FileSaveDialog::accept(0);
        }

    } else {
        // Invalid file name
        MessageBox::show("Invalid file name");
    }
}


static void cb_save_tap()
{

    FileSaveAppendOverwriteDialog *d = WSYSObject::create<FileSaveAppendOverwriteDialog>("Save as", 30, 22, StandardFileFilter::TAPFileFilter());

    d->setStatusLines(2);
    d->setFileName( save__get_requested_name() );
    int r = d->exec();
    if (r<0) {
        d->destroy();
        return;
    }
    if (save__start_save_tap(d->getPath(), r==1?true:false)==0) {
        WSYS_LOGI("Started save file");
    } else {
        // Error.
        MessageBox::show("Cannot start TAP save");
    }
    screen__destroyAll();

    save__notify_save_to_tap();
    wsys__send_command(0xFF);
}

static void cb_save_tzx()
{
}


static void cb_return()
{
    screen__destroyAll();
    save__start_save_physical();
    wsys__send_command(0xFF);
}

static void cb_cancel()
{
    screen__destroyAll();
    save__notify_no_save();
    wsys__send_command(0xFF);
}

