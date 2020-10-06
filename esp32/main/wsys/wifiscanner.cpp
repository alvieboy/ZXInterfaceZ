#include "wifiscanner.h"
#include "wifi.h"

void wscan_reset(void *user)
{
    static_cast<WifiScanner*>(user)->reset();
}

void wscan_apcount(void *user, uint8_t count, size_t ssidlensum)
{
    static_cast<WifiScanner*>(user)->apcount(count, ssidlensum);
}

void wscan_ap(void *user, uint8_t auth, uint8_t channel, int8_t rssi, const char *ssid, size_t ssidlen)
{
    static_cast<WifiScanner*>(user)->ap(auth, channel, rssi, ssid, ssidlen);
}

void wscan_finish(void*user)
{
    static_cast<WifiScanner*>(user)->finish();
}

void WifiScanner::reset()
{
    m_aplist.clear();
}

void WifiScanner::apcount(uint8_t count, size_t ssidlensum)
{
    //m_aplist.reserve(count);
}

void WifiScanner::ap(uint8_t auth, uint8_t channel, int8_t rssi, const char *ssid, size_t ssidlen)
{
    AP api(auth, channel, rssi, ssid, ssidlen);
    m_aplist.push_back(api);
}

void WifiScanner::finish()
{
}
extern "C"  {
    static const wifi_scan_parser_t scan_parser =  {
        .reset = &wscan_reset,
        .apcount = &wscan_apcount,
        .ap = &wscan_ap,
        .finish =&wscan_finish
    };
};

int WifiScanner::scan()
{
    return wifi__scan(&scan_parser, this);
};


