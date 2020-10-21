#include "nmimenu.h"
#include "wsys.h"
#include "wsys/menuwindow.h"
#include "wsys/screen.h"
#include "wsys/filechooserdialog.h"
#include "settingsmenu.h"
#include "fileaccess.h"
#include "fpga.h"
#include "sna.h"
#include "wsys/messagebox.h"
#include "tapeplayer.h"
#include "about.h"

static MenuWindow *nmimenu;

static const MenuEntryList nmimenu_entries = {
    .sz = 8,
    .entries = {
        { .flags = 0, .string = "Load snapshot..." },
        { .flags = 1, .string = "Save snapshot..." },
        { .flags = 0, .string = "Play tape..." },
        { .flags = 1, .string = "Poke..." },
        { .flags = 0, .string = "Settings..." },
        { .flags = 0, .string = "Reset" },
        { .flags = 0, .string = "About..." },
        { .flags = 0, .string = "Exit" },
    }
};

static const char *nmimenu_help[] = {
    "Loads a ZX snapshot from file on the SD card",
    "Saves current status as snapshot to SD card",
    "Play a TAP or TZX file. Ensure you load \"\" before",
    "Pokes stuff around",
    "Change wireless, bluetooth, usb settings",
    "Reset the ZX spectrum",
    "About the ZX spectrum",
    "Exit to ZX spectrum"
};

static void cb_load_snapshot();
static void cb_load_tape();
static void cb_exit_nmi();
static void cb_about();
static void cb_reset();
static void cb_settings();

static const CallbackMenu::Function nmimenu_functions[] =
{
    &cb_load_snapshot,
    NULL,
    &cb_load_tape,
    NULL,
    &cb_settings,
    &cb_reset,
    &cb_about,
    &cb_exit_nmi,
};


static void about__show()
{
    AboutWindow *about = WSYSObject::create<AboutWindow>();
    screen__addWindowCentered(about);
    about->setVisible(true);
}

void nmimenu__show()
{
    nmimenu = WSYSObject::create<MenuWindow>("ZX Interface Z", 24, 14);

    nmimenu->setEntries( &nmimenu_entries );
    nmimenu->setCallbackTable( nmimenu_functions );
    nmimenu->setWindowHelpText("Use Q/A to move, ENTER select");
    nmimenu->setStatusLines(2);
    nmimenu->setHelpStrings( nmimenu_help );
    screen__addWindowCentered( nmimenu );
    nmimenu->setVisible(true);
}

static void do_load_snapshot(FileChooserDialog *d, int status)
{
    if (status==0) {
        //FileChooserDialog *d = static_cast<FileChooserDialog*>(data);
        WSYS_LOGI("Snapshot is: %s", d->getSelection());
        snatype_t ret = sna__load_snapshot_extram(d->getSelection());
        WSYS_LOGI("Load Snapshot %d", ret);
        if (ret == SNAPSHOT_ERROR) {
            WSYS_LOGE("Error loading SNA");
            MessageBox::show("Cannot load SNA");
        } else {
            WSYS_LOGI("Starting SNA");
            if (ret==SNAPSHOT_SNA) {
                wsys__send_command(0xFE);
            } else {
                wsys__send_command(0xFD); // Z80 snapshot
            }
        }
    }
}

static void cb_load_snapshot()
{
    FileChooserDialog *dialog = WSYSObject::create<FileChooserDialog>("Load snapshot", 24, 18);
    dialog->setWindowHelpText("Use Q/A to move, ENTER selects");
    dialog->setFilter(FILE_FILTER_SNAPSHOTS);
    if (dialog->exec()>=0) {
        do_load_snapshot(dialog, dialog->result());
    }
    dialog->destroy();
    //&do_load_snapshot, dialog);
    
}

static void do_load_tape(FileChooserDialog *d, int status)
{
    if (status==0) {
        WSYS_LOGI("Tape is: %s", d->getSelection());
        tapeplayer__play(d->getSelection());
        screen__destroyAll();
        wsys__send_command(0xFF);
    }
}



static void cb_load_tape()
{
    FileChooserDialog *dialog = WSYSObject::create<FileChooserDialog>("Load tape", 24, 18);
    dialog->setWindowHelpText("Use Q/A to move, ENTER selects");
    dialog->setFilter(FILE_FILTER_TAPES);
    if (dialog->exec()>=0) {
        do_load_tape(dialog, dialog->result());
    }
    dialog->destroy();
}


static void cb_settings(void)
{
    settings__show();
}

static void cb_exit_nmi()
{
    wsys__send_command(0xFF);
}
#include "inputdialog.h"
static void cb_reset()
{
    screen__destroyAll();
    fpga__reset_spectrum();
}

static void cb_about(void)
{
    about__show();
}
