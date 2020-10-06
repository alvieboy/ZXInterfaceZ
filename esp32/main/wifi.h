#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>
#include "json.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct channel_info {
        uint16_t chan;
        uint16_t freq;
    };

    struct channel_list {
        uint8_t num_chans;
        struct channel_info info[];
    };

extern const struct channel_list wifi_channels;

typedef enum {
    WIFI_SCANNING = 0,
    WIFI_DISCONNECTED,
    WIFI_ACCEPTING,
    WIFI_CONNECTING,
    WIFI_WAIT_IP_ADDRESS,
    WIFI_CONNECTED
} wifi_status_t;



void wifi__init(void);
bool wifi__isconnected(void);
bool wifi__issta(void);
bool wifi__scanning(void);
void wifi__get_conf_json(cJSON *node);
int wifi__get_clients(void);
int wifi__get_ip_info(uint32_t *addr, uint32_t *netmask, uint32_t *gw);
int wifi__get_ap_channel(void);

int wifi__get_ap_ssid(char *dest, unsigned size);
int wifi__get_ap_pwd( char *dest, unsigned size);
int wifi__get_sta_ssid(char *dest, unsigned size);

int wifi__start_scan(void);

wifi_status_t wifi__get_status(void);
const char *wifi__status_string(wifi_status_t status);
int wifi__get_rssi(void);

typedef struct {
    void (*reset)(void *user);
    void (*apcount)(void *user, uint8_t apcount, size_t ssidlensum);
    void (*ap)(void *user, uint8_t auth, uint8_t channel, int8_t rssi, const char *ssid, size_t ssidlen);
    void (*finish)(void*user);
} wifi_scan_parser_t;

int wifi__scan( const wifi_scan_parser_t *parser, void *data );
cJSON *wifi__ap_get_json(void);
int wifi__scan_json(void);

int wifi__config_sta(const char *ssid, const char *pwd);
int wifi__config_ap(const char *ssid, const char *pwd, uint8_t channel);

extern char wifi_ssid[33];

#ifdef __cplusplus
}
#endif

#endif

