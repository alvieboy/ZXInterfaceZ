#include "nmimenu.h"
#include "wsys.h"
#include "wsys/menuwindow.h"
#include "wsys/screen.h"
#include "wsys/filechooserdialog.h"
#include "wsys/chooserdialog.h"
#include "settingsmenu.h"
#include "fileaccess.h"
#include "sna.h"
#include "wsys/messagebox.h"
#include "tapeplayer.h"
#include "about.h"
#include "fasttap.h"
#include "inputdialog.h"
#include "poke.h"
#include "nmi_poke.h"
#include "standardfilefilter.h"
#include "spectctrl.h"
#include "debugger.h"
#include "debugwindow.h"

static MenuWindow *nmimenu;

static MenuEntryList nmimenu_entries = {
    .sz = 10,
    .entries = {
        { .flags = 0, .string = "Load snapshot..." },
        { .flags = 1, .string = "Save snapshot..." },
        { .flags = 1, .string = "Play tape..." },
        { .flags = 0, .string = "Play tape (fast)..." },
        { .flags = 0, .string = "Poke..." },
        { .flags = 0, .string = "Settings..." },
        { .flags = 0, .string = "Reset" },
        { .flags = 0, .string = "Debug" },
        { .flags = 0, .string = "About..." },
        { .flags = 0, .string = "Exit" },
    }
};

static const char *nmimenu_help[] = {
    "Loads a ZX snapshot from file on the SD card",
    "Saves current status as snapshot to SD card",
    "Play a TAP or TZX file.",
    "Play TAP file (fast load)",
    "Pokes stuff around",
    "Change wireless, bluetooth, USB settings",
    "Reset the ZX spectrum",
    "Debug",
    "About the ZX spectrum",
    "Exit to ZX spectrum"
};

static void cb_load_snapshot();
static void cb_load_tape();
static void cb_load_tape_fast();
static void cb_exit_nmi();
static void cb_about();
static void cb_reset();
static void cb_settings();
static void cb_poke();
static void cb_debug();

static const CallbackMenu::Function nmimenu_functions[] =
{
    &cb_load_snapshot,
    NULL,
    &cb_load_tape,
    &cb_load_tape_fast,
    &cb_poke,
    &cb_settings,
    &cb_reset,
    &cb_debug,
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
    nmimenu = WSYSObject::create<MenuWindow>("ZX Interface Z", 24, 16);

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

#include "poke.h"

static void poke_add_entry(void* list, const char *file)
{
    static_cast<EntryList*>(list)->push_back( Entry(file, NULL) );
}

static int poke_ask_fun(void *user)
{
    unsigned long v;
    char *endp = NULL;
    bool retry;

    int r = -1;

    do {
        retry = false;
        InputDialog *in = WSYSObject::create<InputDialog>("Enter POKE value", 28, 5);
        if (in->exec()==0) {
            // Check if valid number
            ESP_LOGI("POKE","Converting '%s'\n", in->getText());

            v = strtoul(in->getText(), &endp, 0);

            in->destroy();

            if (endp!=NULL && (*endp)=='\0') {
                r = 0;
            } else {
                r = -1;
            }
            if (v > 255) {
                r=-1;
            }
            if (r<0) {
                MessageBox::show("Invalid value!");
                retry = true;
            }
            r = v;

        } else {
            r = -1;

            in->destroy();

        }
    } while (retry);
    return r;
}

static void do_select_poke(FileChooserDialog *d, int status)
{
    EntryList list;
    nmi_handler_poke_t nmipoke;
    poke_t poke;
    int r;

    poke__init(&poke);

    r = poke__openfile(&poke, d->getSelection());

    if (r==0) {

        if (poke__loadentries(&poke, &poke_add_entry, &list)<0) {
            MessageBox::show("Cannot load POK entries");
        } else {
    
            ChooserDialog *ch = WSYSObject::create<ChooserDialog>("Choose trainer", 30, 12);

            ch->setEntries(list);

            if (ch->exec()==0) {

                nmi_poke__init(&nmipoke);

                poke__setaskfunction(&poke, &poke_ask_fun, NULL);
                poke__setmemorywriter(&poke, &nmi_poke__mem_write_fun, &nmipoke);

                if (poke__apply_trainer(&poke, ch->getSelection())!=0) {
                    MessageBox::show("Cannot apply POK trainer");
                } else {
                    // All good.
                    nmi_poke__finish(&nmipoke);
                    screen__destroyAll();
                    wsys__send_command(0xFC);
                    return;
                }
            }

            //ch->destroy();
        }
    } else {
        MessageBox::show("Cannot load POK");
        return;
    }

    poke__close(&poke);
}

static void cb_load_snapshot()
{
    FileChooserDialog *dialog = WSYSObject::create<FileChooserDialog>("Load snapshot", 24, 18, StandardFileFilter::AllSnapshotsFileFilter());
    dialog->setWindowHelpText("Use Q/A to move, ENTER selects");
    if (dialog->exec()>=0) {
        do_load_snapshot(dialog, dialog->result());
    }
    //dialog->destroy();
    //&do_load_snapshot, dialog);
    
}

static void cb_poke()
{
    FileChooserDialog *dialog = WSYSObject::create<FileChooserDialog>("Load POKE", 24, 18, StandardFileFilter::AllPokesFileFilter());
    dialog->setWindowHelpText("Use Q/A to move, ENTER selects");
    if (dialog->exec()>=0) {
        do_select_poke(dialog, dialog->result());
    }
    //dialog->destroy();
}


static void do_load_tape(FileChooserDialog *d, int status)
{
    if (status==0) {
        WSYS_LOGI("Tape is: %s", d->getSelection());
        tapeplayer__play_file(d->getSelection());
        screen__destroyAll();
        wsys__send_command(0xFF);
    }
}

static int do_load_tape_fast(FileChooserDialog *d, int status)
{
    char fp[128];
    if (status==0) {
        WSYS_LOGI("Tape is: %s", d->getSelection());

        fullpath(d->getSelection(), fp, 127);
        if (fasttap__prepare_from_file(fp)==0) {
            screen__destroyAll();
            wsys__send_command(0xFF);
            return 0;
        } else {
            MessageBox::show("Cannot load TZX file");
        }
    }
    return -1;
}



static void cb_load_tape()
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
    FileChooserDialog *dialog = WSYSObject::create<FileChooserDialog>("Load tape (fast)", 24, 18, StandardFileFilter::AllTapesFileFilter());
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
    spectctrl__reset();
}

static void cb_about(void)
{
    about__show();
}

static void cb_debug(void)
{
    struct nmi_cpu_context_extram data;
    if (debugger__load_context_from_extram(&data)<0) {
        WSYS_LOGE("Cannot load context");
        return;
    }

    DebugWindow *w = WSYSObject::create<DebugWindow>(data);

    screen__addWindowCentered(w);
    w->setVisible(true);


    //debugger__dump();
}
