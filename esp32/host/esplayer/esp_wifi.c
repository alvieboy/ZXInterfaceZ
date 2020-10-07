#include "esp_wifi.h"
#include <string.h>

static wifi_init_config_t current_config;
static wifi_mode_t wifi_mode;

extern int do_hw_wifi_scan(void);

esp_event_base_t WIFI_EVENT="wifi";

esp_err_t esp_wifi_deinit()
{
    return 0;
}

esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{
    memcpy(&current_config, config, sizeof(current_config));
    return 0;
}

esp_err_t esp_wifi_set_protocol(wifi_interface_t ifx, uint8_t protocol_bitmap)
{
    return 0;
}

esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block)
{
    do_hw_wifi_scan();
    return 0;
}

esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf)
{
    return 0;
}

esp_err_t esp_wifi_set_mode(wifi_mode_t mode)
{
    wifi_mode = mode;
    return 0;
}


esp_err_t esp_wifi_start()
{
    switch (wifi_mode){
    case WIFI_MODE_AP:
        wifi_task_apmode();
        break;
    case WIFI_MODE_STA:
        wifi_task_connect();
        break;
    }
    return 0;
}

esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info)
{
    memset(ap_info->bssid, 0, 6);
    strcpy((char*)ap_info->ssid, "Ap WPA2");

    ap_info->primary = 1;
    ap_info->second = 10;
    ap_info->rssi = 50;
    ap_info->authmode = WIFI_AUTH_WPA2_PSK;
    ap_info->pairwise_cipher = WIFI_CIPHER_TYPE_TKIP_CCMP;
    ap_info->group_cipher = WIFI_CIPHER_TYPE_TKIP_CCMP;
    ap_info->ant = 0;
    ap_info->phy_11b = 0;
    ap_info->phy_11g = 1;
    ap_info->phy_11n = 1;
    ap_info->phy_lr = 0;
    ap_info->wps = 0;


    return 0;
}

esp_err_t esp_wifi_stop()
{
    return 0;
}

esp_err_t esp_wifi_connect()
{
    return 0;
}

esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void *esp_netif)
{
}

int esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records)
{
    memset(ap_records[0].bssid, 0, 6);
    strcpy((char*)ap_records[0].ssid, "Ap WPA2");
    ap_records[0].primary = 1;
    ap_records[0].second = 10;
    ap_records[0].rssi = 50;
    ap_records[0].authmode = WIFI_AUTH_WPA2_PSK;
    ap_records[0].pairwise_cipher = WIFI_CIPHER_TYPE_TKIP_CCMP;
    ap_records[0].group_cipher = WIFI_CIPHER_TYPE_TKIP_CCMP;
    ap_records[0].ant = 0;
    ap_records[0].phy_11b = 0;
    ap_records[0].phy_11g = 1;
    ap_records[0].phy_11n = 1;
    ap_records[0].phy_lr = 0;
    ap_records[0].wps = 0;

    *number = 1;
    return 0;
}

esp_err_t esp_wifi_ap_get_sta_list(wifi_sta_list_t *sta)
{
    sta->num = 0;
    return 0;
}

