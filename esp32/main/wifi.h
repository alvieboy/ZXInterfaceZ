#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>
#include "json.h"

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

void wifi__init(void);
bool wifi__isconnected(void);
bool wifi__issta(void);
bool wifi__scanning(void);
void wifi__get_conf_json(cJSON *node);
int wifi__get_clients();
int wifi__get_ip_info(uint32_t *addr, uint32_t *netmask, uint32_t *gw);
int wifi__get_ap_channel();

int wifi__get_ap_ssid(char *dest, unsigned size);
int wifi__get_ap_pwd( char *dest, unsigned size);

typedef struct {
    void (*reset)(void *user);
    void (*apcount)(void *user, uint8_t apcount, size_t ssidlensum);
    void (*ap)(void *user, uint8_t auth, uint8_t channel, const char *ssid, size_t ssidlen);
    void (*finish)(void*user);
} wifi_scan_parser_t;

int wifi__scan( const wifi_scan_parser_t *parser, void *data );
cJSON *wifi__ap_get_json();
int wifi__scan_json();

int wifi__config_sta(const char *ssid, const char *pwd);
int wifi__config_ap(const char *ssid, const char *pwd, uint8_t channel);

extern char wifi_ssid[33];

#ifdef __cplusplus
}
#endif

#endif

