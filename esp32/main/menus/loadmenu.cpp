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
#include "../resource.h"

static void cb_load_tape_fast();
static void cb_load_tape_slow();
static void cb_return_to_standard_tape();
static void cb_cancel();

static MenuEntryList load_entries = {
    .sz = 3,
    .entries = {
        { .flags = 0, .string = "Load from standard tape" },
        { .flags = 0, .string = "Fast load from TAP/TZX" },
        { .flags = 0, .string = "Load from internal audio" },
//        { .flags = 0, .string = "Cancel load" },
    }
};

static const char *loadmenu_help[] = {
    "Load from standard cassette tape",
    "Fast load from a TAP/TZX file",
    "Load from internal audio",
    "Cancel load"
};

static const CallbackMenu::Function load_functions[] =
{
    &cb_return_to_standard_tape,
    &cb_load_tape_fast,
    &cb_load_tape_slow,
    &cb_cancel
};


void loadmenu__show()
{
    WSYS_LOGI("LOAD menu displaying");
    MenuWindow *loadmenu = WSYSObject::create<MenuWindow>("LOAD", 26, 15);

    // Enable or disable ULA override based on model
    if (model__supports_ula_override())
        load_entries[2].flags = 0;
    else
        load_entries[2].flags = 1;


    loadmenu->setEntries( &load_entries );
    loadmenu->setCallbackTable( load_functions );
    loadmenu->setWindowHelpText("Use Q/A to move, ENTER select");
    loadmenu->setStatusLines(2);
    loadmenu->setHelpStrings( loadmenu_help );
    screen__addWindowCentered( loadmenu );
    loadmenu->setVisible(true);
}


static int do_load_tape_fast(FileChooserDialog *d, int status)
{
    char fp[128];
    if (status==0) {
        WSYS_LOGI("Tape is: %s", d->getSelection());

        fullpath(d->getSelection(), fp, 127);
        if (fasttap__prepare_from_file(fp)==0) {
            screen__destroyAll();

            // Preload status
            uint8_t status = 0x00;
            fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);

            wsys__send_command(0xFF);
            return 0;
        } else {
            MessageBox::show("Cannot load TZX file");
        }
    }
    return -1;
}


static void do_load_tape(FileChooserDialog *d, int status)
{
    if (status==0) {
        WSYS_LOGI("Tape is: %s", d->getSelection());
        tapeplayer__play_file(d->getSelection());
        cb_return_to_standard_tape();
    }
}

static void cb_load_tape_slow()
{
    FileChooserDialog *dialog = WSYSObject::create<FileChooserDialog>("Load tape", 24, 18, StandardFileFilter::AllTapesFileFilter());
    dialog->setWindowHelpText("Use Q/A to move, ENTER selects");
    if (dialog->exec()>=0) {
        do_load_tape(dialog, dialog->result());
    }
    //dialog->destroy();
}

static void cb_load_tape_fast()
{
    FileChooserDialog *dialog = WSYSObject::create<FileChooserDialog>("Load tape (fast)", 24, 18, StandardFileFilter::AllTapesAndScreensFileFilter());
    dialog->setWindowHelpText("Use Q/A to move, ENTER selects");
    do {
        if (dialog->exec()>=0) {
            if (do_load_tape_fast(dialog, dialog->result())==0)
                break;
        } else {
            break;
        }
    } while (1);
    //dialog->destroy();
}


static void cb_return_to_standard_tape()
{
    screen__destroyAll();

    // Preload status
    uint8_t status = 0x00;
    fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);

    wsys__send_command(0xFF);
}

static void cb_cancel()
{
    screen__destroyAll();
    // Preload status
    uint8_t status = 0xFF;
    fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);

    wsys__send_command(0xFF);
}

