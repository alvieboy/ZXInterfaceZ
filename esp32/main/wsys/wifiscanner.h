#pragma once

#include "inttypes.h"
#include <stdlib.h>
#include <string>
#include <vector>

class WifiScanner
{
public:

    struct AP {
        AP() {
            m_descr[0] = '\0';
        }
        AP(uint8_t auth, uint8_t channel, int8_t rssi,  const char *ssid, size_t ssidlen): m_auth(auth), m_channel(channel), m_rssi(rssi), m_ssid(ssid) {
            sprintf(m_descr,"Channel %d, signal strength %d", channel, rssi);
        }

        const char *str() const { return m_ssid.c_str(); }
        int flags()const { return 0; }
        const std::string &ssid() const { return m_ssid; }
        uint8_t auth() const { return m_auth; }
        const char *getDescription() const { return m_descr; }
    private:
        uint8_t m_auth;
        uint8_t m_channel;
        int8_t m_rssi;
        std::string m_ssid;
        char m_descr[36];
    };

    int scan();
    void reset();
    void apcount(uint8_t count, size_t ssidlensum);
    void ap(uint8_t auth, uint8_t channel, int8_t rssi,  const char *ssid, size_t ssidlen);
    void finish();
    void sort_rssi();
    const std::vector<AP> &aplist() const { return m_aplist; }
protected:
    std::vector<AP> m_aplist;
};

