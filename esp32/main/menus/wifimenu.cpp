#include "wifimenu.h"
#include "object_signal.h"
#include "wifi.h"
#include "screen.h"
#include "editbox.h"
#include "fixedlayout.h"
#include "color.h"
#include "spectrum_kbd.h"
#include "messagebox.h"
#include "wifiscanner.h"
#include "inputdialog.h"

static const MenuEntryList wifimenu_entries = {
    .sz = 6,
    .entries = {
        { .flags = 1, .string = "Status" },
        { .flags = 1, .string = "Mode" },
        { .flags = 1, .string = "Wireless" },
        { .flags = 1, .string = "Network" },
        { .flags = 1, .string = "Advanced" },
        { .flags = 0, .string = "Back" },
    }
};

static const MenuEntryList wifimodeselectormenu = {
    .sz = 3,
    .entries = {
        { .flags = 0, .string = "Access Point mode" },
        { .flags = 0, .string = "Station mode" },
        { .flags = 0, .string = "Back" }
    }
};


class WifiWirelessSettingsAP: public FixedLayout
{
public:
    WifiWirelessSettingsAP() {
        char temp[32];
        m_channel = wifi__get_ap_channel();

        wifi__get_ap_ssid(temp,sizeof(temp));
        m_ssid = new EditBox(temp);

        wifi__get_ap_pwd(temp,sizeof(temp));
        m_password = new EditBox(temp);

        m_chan = new Label("");

        updateChannelText();

        m_chan->setBackground( MAKECOLOR(WHITE, BLUE) );

        addChild(m_ssid, 0, 3, 21, 1);
        addChild(m_password, 0, 7, 21, 1);
        addChild(m_chan, 0, 11, 21, 1);

        m_ssid->enter().connect( this, &WifiWirelessSettingsAP::ssidEnter);
        m_password->enter().connect( this, &WifiWirelessSettingsAP::passwordEnter);
        m_channellist = NULL;
    }

    ~WifiWirelessSettingsAP()
    {
        if(m_channellist) {
            delete(m_channellist);
            free(m_chandata);
        }
    }

    virtual void drawImpl()  {
        parentDrawImpl();
        screenptr_t screenptr = m_screenptr;
        screenptr.drawstring("Access point settings");
        screenptr.nextcharline(2);
        screenptr.drawstring("SSID:");
        screenptr.nextcharline(2);
        drawthumbstring(screenptr, "Press [S] to change SSID");
        screenptr.nextcharline(2);
        screenptr.drawstring("Password:");
        screenptr.nextcharline(2);
        drawthumbstring(screenptr, "Press [P] to change password");
        screenptr.nextcharline(2);
        screenptr.drawstring("Channel:");
        screenptr.nextcharline(2);
        drawthumbstring(screenptr, "Press [C] to change channel");
    }

    virtual bool handleLocalEvent(uint8_t type, u16_8_t code) override
    {
        bool handled = false;
        if (type!=0)
            return handled;

        char c = spectrum_kbd__to_ascii(code.v);
        switch (c) {
        case 'S': /* fall-through */
        case 's':
            handled = true;
            m_ssid->setEditable(true);
            break;
        case 'P': /* fall-through */
        case 'p':
            handled = true;
            m_password->setEditable(true);
            break;
        case 'C': /* fall-through */
        case 'c':
            handled = true;
            showChannelPopup();
            break;
        default:
            break;
        }
        return handled;
    }

    void ssidEnter()
    {
        m_ssid->setEditable(false);
    }

    void passwordEnter()
    {
        m_password->setEditable(false);
    }

    static void getChannelString(const struct channel_info &info, char *dest)
    {
        sprintf(dest,"%d (%d MHz)",
                info.chan,
                info.
                freq);
    }

    void buildChannelList()
    {
        const struct channel_list *list = &wifi_channels;
        int i;

        class strwrap: public std::string
        {
        public:
            strwrap(const char *c): std::string(c) {}
            const char *str() const { return c_str(); }
            int flags() const { return 0; }
        };

        std::vector<strwrap> chanlist;

        chanlist.reserve(list->num_chans);

        for (i=0;i<list->num_chans;i++) {
            char tempstr[32];
            getChannelString(list->info[i], tempstr);
            chanlist.push_back(tempstr);
        }

        m_channellist = Menu::allocEntryList( chanlist.begin(), chanlist.end(), m_chandata);
    }

    void channelSelected(uint8_t index)
    {
        const struct channel_list *list = &wifi_channels;
        screen__removeWindow(m_chanmenu);
        m_chanmenu = NULL;
        m_channel = list->info[index].chan;
        updateChannelText();
    }

    const struct channel_info *getChannelInfo(int chan, int *index=NULL)
    {
        const struct channel_list *list = &wifi_channels;
        int i;
        for (i=0;i<list->num_chans;i++) {
            if (chan == list->info[i].chan) {
                if (index)
                    *index = i;
                return &list->info[i];
            }
        }
        if (index)
            *index=-1;
        return NULL;
    }

    void updateChannelText()
    {
        char tempstr[32];
        const struct channel_info *info = getChannelInfo(m_channel);
        if (info!=NULL) {
            getChannelString( *info, tempstr );
        } else {
            strcpy(tempstr,"Unknown");
        }
        m_chan->setText( tempstr );
    }

    void showChannelPopup()
    {
        int index;

        if (!m_channellist) {
            buildChannelList();
        }

        m_chanmenu = new MenuWindowIndexed("Channel", 16, 10);

        m_chanmenu->setEntries(m_channellist);
        m_chanmenu->selected().connect( this, &WifiWirelessSettingsAP:: channelSelected);

        getChannelInfo(m_channel, &index);
        if (index>=0) {
            m_chanmenu->setActiveEntry(index);
        }

        screen__addWindowCentered(m_chanmenu);

    }

private:
    EditBox *m_ssid;
    EditBox *m_password;
    Label *m_chan;
    MenuWindowIndexed *m_chanmenu;
    MenuEntryList *m_channellist;
    void *m_chandata;
    int m_channel;

};

class WifiWirelessSettingsSTA: public FixedLayout
{
public:
    WifiWirelessSettingsSTA()
    {
        m_ssid = new Label("");
        m_status = new Label("");
        m_rssi = new Label("");
        m_scan = new Button("Scan WiFi network");

        m_ssid->setBackground( MAKECOLOR(WHITE, BLUE) );

        addChild(m_ssid, 0, 3, 21, 1);
        addChild(m_status, 1, 7, 21, 1);
        addChild(m_rssi, 11, 9, 11, 1);
        addChild(m_scan, 0, 15, 21, 1);
        m_scan->clicked().connect(this, &WifiWirelessSettingsSTA::doScan);
    }


    void doScan()
    {
        notifyslot = wsys__subscribesystemevent(this, &WifiWirelessSettingsSTA::systemEvent);

        int r = m_scanner.scan();

        if (r==0) {
            m_scanmessage = new MessageBox("Scanning WiFi", 22, 6);
            m_scanmessage->setText("Scanning WiFi...");
            m_scanmessage->setButtonText("Cancel [ENTER]");
            m_scanmessage->exec();
        } else {
            wsys__unsubscribesystemevent(notifyslot);
            // notifyslot
            MessageBox::show("Cannot start scan");
        }
    }

    int notifyslot;

    void systemEvent(const systemevent_t &event)
    {
        if (event.type == SYSTEMEVENT_TYPE_WIFI && event.event==SYSTEMEVENT_WIFI_SCAN_COMPLETED) {
            WSYS_LOGI("SYSTEM EVENT");
            if (m_scanmessage) {
                scanCompleted();
            }
        }
    }


    virtual void drawImpl()  {
        parentDrawImpl();
        screenptr_t screenptr = m_screenptr;
        screenptr.drawstring("WiFi settings");
        screenptr.nextcharline(2);
        screenptr.drawstring("Connected to:");
        screenptr.nextcharline(3);
        screenptr.drawstring("Current status:");
        screenptr.nextcharline(11);
        drawthumbstring(screenptr, "Press [ENTER] to scan network");
        updateStatus();
    }

    void updateStatus()
    {
        char ssid[32];
        wifi_status_t status = wifi__get_status();
        m_status->setText( wifi__status_string(status));
        switch(status) {
        case WIFI_WAIT_IP_ADDRESS:
            /* Fall-through */
        case WIFI_CONNECTED:
            /* Fall-through */
            wifi__get_sta_ssid(ssid, sizeof(ssid));
            m_ssid->setText( ssid );
            break;
        case WIFI_CONNECTING:
            m_ssid->setText("");
            break;
        default:
            break;
        }
    }

    void scanCompleted()
    {
        WSYS_LOGI("scan Completed, showing list (%d)", m_scanner.aplist().size());
        m_scanmessage->destroy();
        m_scanmessage = NULL;

        if (m_scanner.aplist().size()==0) {
            MessageBox::show("No access points found");
        } else {
            m_apchooser = new MenuWindowIndexed("Choose WiFi", 30, 16);
            m_apchooser->setEntries( Menu::allocEntryList(
                                                          m_scanner.aplist().begin(),
                                                          m_scanner.aplist().end(),
                                                          m_apchooserdata)
                                   );
            m_apchooser->setWindowHelpText("Press SPACE to go back");
            m_apchooser->selected().connect(this, &WifiWirelessSettingsSTA::apSelected);
            screen__addWindowCentered(m_apchooser);
        }
    }

    void apSelected(uint8_t index)
    {
        screen__removeWindow(m_apchooser);

        if (index==0xff){
            return;
        }

        WSYS_LOGI("Connecting to AP '%s'", m_scanner.aplist()[index].ssid().c_str());
        WSYS_LOGI(" Auth mode: %d", m_scanner.aplist()[index].auth());

        // If we need a password, ask for it
        if (m_scanner.aplist()[index].auth() != WIFI_AUTH_OPEN) {
            m_pwdchooser = new InputDialog("Enter password", 24, 6);
            m_pwdchooser->setLabel("Enter WiFi password:");
            if (m_pwdchooser->exec()==0) {
                WSYS_LOGI("Connecting to AP with password");
                wifi__config_sta(m_scanner.aplist()[index].ssid().c_str(), m_pwdchooser->getText());

            }
            m_pwdchooser->destroy();
        } else {
            wifi__config_sta(m_scanner.aplist()[index].ssid().c_str(), NULL);
        }

        //screen__removeWindow(this);
    }

    Label *m_ssid;
    Label *m_status;
    Label *m_rssi;
    Button *m_scan;
    MessageBox *m_scanmessage;
    WifiScanner m_scanner;
    MenuWindowIndexed *m_apchooser;
    InputDialog *m_pwdchooser;
    void *m_apchooserdata;
};

class WifiWirelessSettings: public StackedWidget
{
public:
    WifiWirelessSettings() {
        m_sta = new WifiWirelessSettingsSTA();
        m_ap = new WifiWirelessSettingsAP();
        addChild(m_sta);
        addChild(m_ap);
    }

    virtual void draw(bool force) override {
        if (wifi__issta()) {
            setCurrentIndex(0);
        } else {
            setCurrentIndex(1);
        }
        StackedWidget::draw(force);
    }
private:
    WifiWirelessSettingsAP *m_ap;
    WifiWirelessSettingsSTA *m_sta;
};




WifiMenu::WifiMenu(): Window("Wifi settings", 32, 20)
{
    m_hl = new HLayout();
    setChild(m_hl);
    m_menu = new IndexedMenu();
    m_hl->addChild(m_menu);

    m_hl->addChild( new VBar() );

    m_stack = new StackedWidget();

    m_status = new WifiStatus();
    m_mode   = new WifiMode();
    m_wifisettings = new WifiWirelessSettings();

    m_stack->addChild(m_status);
    m_stack->addChild(m_mode);
    m_stack->addChild(m_wifisettings);
    m_stack->addChild(new Label("Opt3"));
    m_stack->addChild(new Label("Opt4"));
    m_stack->addChild(new Label(""));


    m_hl->addChild(m_stack, LAYOUT_FLAG_HEXPAND);
    m_menu->setEntries(&wifimenu_entries);

    m_menu->selectionChanged().connect( this, &WifiMenu::selected ) ;    m_menu->selected().connect( this, &WifiMenu::activated ) ;
    m_mode->modechanged().connect( this, &WifiMenu::modechanged);
}

void WifiMenu::modechanged()
{
    WSYS_LOGI("Mode changed, need reload window");
    screen__removeWindow(this);
}

void WifiMenu::selected(uint8_t index)
{
    m_stack->setCurrentIndex(index);
}

void WifiMenu::activated(uint8_t index)
{
    switch(index) {
    case 5:
        screen__removeWindow(this);
        break;
    }
}


/* ********************************************************************************************** */

void WifiStatus::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    screenptr_t temp;
    uint32_t addr;
    uint32_t netmask;
    uint32_t gw;

    parentDrawImpl();

    screenptr.drawstring("WiFi status: ");


    screenptr.nextcharline();
    temp = screenptr;
    screenptr++;

    if (wifi__issta()) {
        screenptr.drawstring("STA");
        screenptr = temp;
        screenptr.nextcharline();
    } else {
        screenptr.drawstring("Access Point mode");

        screenptr = temp;
        screenptr.nextcharline();
        screenptr.nextcharline();

        screenptr.printf("Clients: %d", wifi__get_clients());
        screenptr.nextcharline();
    }

    screenptr.nextcharline();
    screenptr.drawstring("IP configuration:");
    screenptr.nextcharline();
    screenptr.nextcharline();

    int r = wifi__get_ip_info(&addr,&netmask,&gw);
    if (r==0) {
        screenptr.printf("IP  : %d.%d.%d.%d",
                         (addr>>24)&0xff,
                         (addr>>16)&0xff,
                         (addr>>8)&0xff,
                         (addr&0xff));

        screenptr.nextcharline();

        screenptr.printf("Mask: %d.%d.%d.%d",
                         (netmask>>24)&0xff,
                         (netmask>>16)&0xff,
                         (netmask>>8)&0xff,
                         (netmask&0xff));

        screenptr.nextcharline();

        screenptr.printf("GW  : %d.%d.%d.%d",
                         (gw>>24)&0xff,
                         (gw>>16)&0xff,
                         (gw>>8)&0xff,
                         (gw&0xff));

    } else {
        screenptr.drawstring("<no information>");
    }

}

/* ********************************************************************************************** */

void WifiModeText::drawImpl()
{
    screenptr_t screenptr = m_screenptr;
    screenptr_t temp = screenptr;

    parentDrawImpl();

    screenptr.drawstring("Current mode:");

    screenptr.nextcharline();
    temp = screenptr;
    screenptr++;

    if (wifi__issta()) {
        screenptr.drawstring("STA");
    } else {
        screenptr.drawstring("Access Point mode");
    }
    screenptr = temp;
    screenptr.nextcharline();
    screenptr.nextcharline();
    screenptr.drawstring("Press ENTER to ");
    screenptr.nextcharline();
    screenptr.drawstring("change mode");
}


/* ********************************************************************************************** */

WifiMode::WifiMode(Widget *parent): VLayout(parent)
{
    m_text = new WifiModeText();
    m_button = new Button("Change mode");
    addChild(m_text, LAYOUT_FLAG_VEXPAND);
    addChild(m_button);
    m_button->clicked().connect( this, &WifiMode::changeMode );

    systemnotifyslot = wsys__subscribesystemevent(this, &WifiMode::systemEvent);
}

void WifiMode::changeMode() {
    m_modeselwindow = new MenuWindowIndexed("WiFi mode", 20, 6);
    m_modeselwindow->selected().connect( this, &WifiMode::modeSelected );
    m_modeselwindow->setEntries(&wifimodeselectormenu);
    screen__addWindowCentered(m_modeselwindow);
    m_modeselwindow->show();
}

void WifiMode::systemEvent(const systemevent_t &event)
{
    if (event.type==SYSTEMEVENT_TYPE_WIFI && event.event == SYSTEMEVENT_WIFI_STATUS_CHANGED) {
        setdamage(DAMAGE_ALL);
    }
}

void WifiMode::modeSelected(uint8_t val)
{
    bool needreload = false;
    switch (val) {
    case 0:
        if (wifi__issta())
            needreload=true;
        break;
    case 1:
        if (!wifi__issta())
            needreload=true;
        break;
    default:
        break;
    };
    screen__removeWindow(m_modeselwindow);

    if (needreload)
        m_modechanged.emit();
}


