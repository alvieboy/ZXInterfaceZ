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

static MenuWindow *nmimenu;

static const MenuEntryList nmimenu_entries = {
    .sz = 7,
    .entries = {
        { .flags = 0, .string = "Load snapshot..." },
        { .flags = 1, .string = "Save snapshot..." },
        { .flags = 0, .string = "Play tape..." },
        { .flags = 1, .string = "Poke..." },
        { .flags = 0, .string = "Settings..." },
        { .flags = 0, .string = "Reset" },
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
    "Exit to ZX spectrum"
};

static void cb_load_snapshot();
static void cb_load_tape();
static void cb_exit_nmi();
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
    &cb_exit_nmi,
};


void nmimenu__show()
{
    nmimenu = new MenuWindow("ZX Interface Z", 24, 13);

    nmimenu->setEntries( &nmimenu_entries );
    nmimenu->setCallbackTable( nmimenu_functions );
    nmimenu->setWindowHelpText("Use Q/A to move, ENTER select");
    nmimenu->setStatusLines(2);
    nmimenu->setHelpStrings( nmimenu_help );
    screen__addWindowCentered( nmimenu );
    nmimenu->setVisible(true);
}

static void do_load_snapshot(void *data, int status)
{
    if (status==0) {
        FileChooserDialog *d = static_cast<FileChooserDialog*>(data);
        ESP_LOGI("WSYS", "Snapshot is: %s", d->getSelection());
        int ret = sna__load_sna_extram(d->getSelection());
        ESP_LOGI("WSYS", "Load Snapshot %d", ret);
        if (ret==0) {
            ESP_LOGI("WSYS", "Starting SNA");
            wsys__send_command(0xFE);
        } else {
            ESP_LOGI("WSYS", "Error loading SNA");
            MessageBox::show("Cannot load SNA");
        }
    }
}

static void cb_load_snapshot()
{
    FileChooserDialog *dialog = new FileChooserDialog("Load snapshot", 24, 18);
    dialog->setWindowHelpText("Use Q/A to move, ENTER selects");
    dialog->setFilter(FILE_FILTER_SNAPSHOTS);
    dialog->exec(&do_load_snapshot, dialog);
}

static void do_load_tape(void *data, int status)
{
    if (status==0) {
        FileChooserDialog *d = static_cast<FileChooserDialog*>(data);
        ESP_LOGI("WSYS", "Tape is: %s", d->getSelection());
        tapeplayer__play(d->getSelection());
        screen__destroyAll();
        wsys__send_command(0xFF);
    }
}



static void cb_load_tape()
{
    FileChooserDialog *dialog = new FileChooserDialog("Load tape", 24, 18);
    dialog->setWindowHelpText("Use Q/A to move, ENTER selects");
    dialog->setFilter(FILE_FILTER_TAPES);
    dialog->exec(&do_load_tape, dialog);
}


static void cb_settings(void)
{
    settings__show();
}

static void cb_exit_nmi()
{
    wsys__send_command(0xFF);
}

static void cb_reset()
{
    screen__destroyAll();
    fpga__reset_spectrum();
}

