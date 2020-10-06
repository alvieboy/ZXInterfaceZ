#include "esp_netif.h"
#include "esp_wifi_default.h"
#include "esp_event.h"

esp_event_base_t IP_EVENT="ip";

int esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info)
{
    return 0;
}

int esp_netif_dhcps_stop(esp_netif_t *esp_netif)
{
    return 0;
}

void esp_netif_destroy(esp_netif_t *esp_netif)
{
}

esp_netif_t *esp_netif_create_default_wifi_ap(void)
{
    return NULL;
}

esp_netif_t *esp_netif_create_default_wifi_sta(void)
{
    return NULL;
}

esp_err_t esp_netif_init()
{
    return 0;
}

esp_err_t esp_netif_dhcps_start(esp_netif_t *netif)
{
    return 0;
}
