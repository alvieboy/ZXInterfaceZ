#include "esp_wifi.h"
#include "esp_netif_types.h"

esp_err_t esp_wifi_ap_get_sta_list(wifi_sta_list_t *sta)
{
    sta->num = 0;
    return 0;
}

esp_err_t esp_netif_get_ip_info(esp_netif_t *netif, esp_netif_ip_info_t *info)
{
    info->ip.addr = 0x8a8a8a8a;
    info->netmask.addr = 0x00ffffff;
    info->gw.addr = 0x018a8a8a;
    return 0;
}

