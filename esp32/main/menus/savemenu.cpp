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
    WSYS_LOGI("SAVE menu displaying");
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
static void cb_save_tap()
{
}

static void cb_save_tzx()
{
}


static void cb_return()
{
    screen__destroyAll();
    save__notify_no_save();
    wsys__send_command(0xFF);
}

static void cb_cancel()
{
    screen__destroyAll();
    save__notify_no_save();
    wsys__send_command(0xFF);
}

