
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "defs.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "led.h"
#include <string.h>
#include "interfacez_resources.h"
#include "onchip_nvs.h"
#include "mdns.h"
#include "json.h"
#include "wifi.h"

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT (1<<0)
#define WIFI_SCANNING_BIT (1<<1)

#define U32_IP_ADDR(a,b,c,d) \
    ((uint32_t)((d) & 0xff) << 24) | \
    ((uint32_t)((c) & 0xff) << 16) | \
    ((uint32_t)((b) & 0xff) << 8)  | \
    (uint32_t)((a) & 0xff)

static const wifi_scan_parser_t *scan_parser = NULL;
static void *scan_parser_data = NULL;
static esp_netif_t *netif;


const struct channel_list wifi_channels =
{
    12,
    {
        { 1, 2412 },
        { 2, 2417 },
        { 3, 2422 },
        { 4, 2427 },
        { 5, 2432 },
        { 6, 2437 },
        { 7, 2442 },
        { 8, 2447 },
        { 9, 2452 },
        { 10, 2457 },
        { 11, 2462 },
        { 12, 2467 }
    }
};

int wifi__get_ap_channel()
{
    return nvs__u8("ap_chan", 3);
}

int wifi__get_ap_ssid(char *dest, unsigned size)
{
    return nvs__str("ap_ssid",
                    dest, size,
                    EXAMPLE_ESP_WIFI_SSID);
}

int wifi__get_ap_pwd( char *dest, unsigned size)
{
    return nvs__str("ap_pwd",
                    dest,
                    size,
                    EXAMPLE_ESP_WIFI_PASS);
}

#ifndef __linux__

static bool issta = false;
char wifi_ssid[33];

bool wifi__isconnected()
{
    return xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT;
}

bool wifi__scanning()
{
    return xEventGroupGetBits(s_wifi_event_group) & WIFI_SCANNING_BIT;
}

bool wifi__issta()
{
    return issta;
}

static uint32_t wifi__config_get_ip() { return nvs__u32("ip", U32_IP_ADDR(192, 168, 120, 1)); }
static uint32_t wifi__config_get_gw() { return nvs__u32("gw", U32_IP_ADDR(0,0,0,0)); }
static uint32_t wifi__config_get_netmask() { return nvs__u32("mask", U32_IP_ADDR(255,255,255,0)); }

static void setup_mdns()
{
    char hostname[64];

    ESP_ERROR_CHECK(mdns_init());
    nvs__str("hostname",hostname,sizeof(hostname),"interfacez.local");
    ESP_ERROR_CHECK(mdns_instance_name_set("ZX InterfaceZ"));

    ESP_ERROR_CHECK(mdns_hostname_set("interfacez"));

    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    ESP_ERROR_CHECK(mdns_service_instance_name_set("_http", "_tcp", "XZ InterfaceZ Web Interface"));

    ESP_ERROR_CHECK(mdns_service_add(NULL, "_zxictrl", "_tcp", BUFFER_PORT, NULL, 0));
    ESP_ERROR_CHECK(mdns_service_instance_name_set("_zxictrl", "_tcp", "XZ InterfaceZ Control Interface"));
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)

{
    ip_event_got_ip_t* event;

    switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
        event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        led__set(LED2, 1);

        setup_mdns();

        break;
    default:
        ESP_LOGW(TAG,"Unhandled IP event %d", event_id);
        break;
    }
}

static void wifi__parse_scan();

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base != WIFI_EVENT)
        return;

    switch (event_id) {

    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        led__set(LED2, 0);
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case WIFI_EVENT_AP_STACONNECTED:
        {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                     MAC2STR(event->mac), event->aid);
        }
        break;
    case WIFI_EVENT_AP_STADISCONNECTED:
        {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                     MAC2STR(event->mac), event->aid);
        }
        break;

    case WIFI_EVENT_SCAN_DONE:
        wifi__parse_scan();
        xEventGroupClearBits(s_wifi_event_group, WIFI_SCANNING_BIT);
        //case WIFI_EVENT_STA_CONNECTED:
        break;
    case WIFI_EVENT_AP_START:
        ESP_LOGI(TAG,"WiFi AP started");
        setup_mdns();
        break;
    default:
        ESP_LOGW(TAG,"Unhandled WIFI event %d", event_id);

        break;
    }
}

static const wifi_scan_config_t scan_config = {
    .ssid = 0,
    .bssid = 0,
    .channel = 0,
    .show_hidden = false
};

void wifi__init_softap()
{
    if (netif) {
        esp_netif_destroy(netif);
        esp_wifi_stop();
    }

    netif = esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));

    esp_netif_ip_info_t info;
    memset(&info, 0, sizeof(info));

    issta = false;

    mdns_free();

    info.ip.addr = wifi__config_get_ip();
    info.gw.addr = wifi__config_get_gw();
    info.netmask.addr = wifi__config_get_netmask();

    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));

    wifi_config_t wifi_config;

    memset(&wifi_config,0,sizeof(wifi_config));

    wifi_config.ap.channel = wifi__get_ap_channel();
    wifi_config.ap.ssid_len = wifi__get_ap_ssid((char*)&wifi_config.ap.ssid[0],
                                                sizeof(wifi_config.ap.ssid));

    wifi_config.ap.max_connection = EXAMPLE_MAX_STA_CONN;

    int pwdsize = wifi__get_ap_pwd((char*)&wifi_config.ap.password[0],
                                   sizeof(wifi_config.ap.password));

    if (pwdsize == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Configured WiFi in AP mode");
    ESP_LOGI(TAG, " SSID    : %s", wifi_config.ap.ssid);
    ESP_LOGI(TAG, " Password: %s", wifi_config.ap.password);
    ESP_LOGI(TAG, " Channel : %d", wifi_config.ap.channel);
    ESP_LOGI(TAG, " Max conn: %d", wifi_config.ap.max_connection);
}

static void wifi__init_core()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL));

}

void wifi__init_wpa2()
{
    if (netif) {
        ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
        esp_netif_destroy(netif);
        esp_wifi_stop();
    }

    issta = true;

    mdns_free();

    netif = esp_netif_create_default_wifi_sta();
    //ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    nvs__fetch_str("sta_ssid", (char*)&wifi_config.sta.ssid[0], sizeof(wifi_config.sta.ssid), "");
    nvs__fetch_str("sta_pwd", (char*)&wifi_config.sta.password[0], sizeof(wifi_config.sta.password), "");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA,
                                          (WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N)
                                         )
                   );

    strcpy(wifi_ssid, (char*)wifi_config.sta.ssid );

    //esp_wifi_set_ps (WIFI_PS_NONE);
//    ESP_ERROR_CHECK( esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BW_HT20) );

    ESP_LOGI(TAG, "Configured WiFi in STA mode");
    ESP_LOGI(TAG, " SSID    : %s", wifi_config.sta.ssid);

    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi__init()
{
    s_wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi__init_core();
    if (nvs__u8("wifi", WIFI_MODE_AP)==WIFI_MODE_STA) {
        wifi__init_wpa2();
    } else {
        wifi__init_softap();
    }
}

static int wifi__start_scan()
{
    int r = esp_wifi_scan_start(&scan_config, false);
    if (r<0) {
        ESP_LOGE(TAG, "Cannot start scan process: %s", esp_err_to_name(r));
        return r;
    }
    xEventGroupSetBits(s_wifi_event_group, WIFI_SCANNING_BIT);
    return 0;
}



#else
#include <stdbool.h>
#include "json.h"
#include <inttypes.h>
#include "wifi.h"
#include "esp_wifi.h"

void wifi__end_scan()
{
    ESP_LOGI(TAG,"WiFI scan finished");
    wifi__parse_scan();
    xEventGroupClearBits(s_wifi_event_group, WIFI_SCANNING_BIT);

}

void wifi__init()
{
    s_wifi_event_group = xEventGroupCreate();
}

bool wifi__isconnected()
{
    return false;
}

bool wifi__scanning()
{
    return xEventGroupGetBits(s_wifi_event_group) & WIFI_SCANNING_BIT;
}

bool wifi__issta()
{
    return false;
}

void wifi__init_softap()
{
}

void wifi__init_wpa2()
{
}

char wifi_ssid[33] = {0} ;

static uint32_t wifi__config_get_ip() { return nvs__u32("ip", U32_IP_ADDR(192, 168, 120, 1)); }
static uint32_t wifi__config_get_gw() { return nvs__u32("gw", U32_IP_ADDR(0,0,0,0)); }
static uint32_t wifi__config_get_netmask() { return nvs__u32("mask", U32_IP_ADDR(255,255,255,0)); }

extern int do_hw_wifi_scan(void);

int wifi__start_scan()
{
    xEventGroupSetBits(s_wifi_event_group, WIFI_SCANNING_BIT);

    return do_hw_wifi_scan();
}

#endif

void wifi__get_conf_json(cJSON *node)
{
    char temp[64];
    struct ip4_addr addr;

    bool issta = nvs__u8("wifi", WIFI_MODE_AP)==WIFI_MODE_STA;
    cJSON_AddStringToObject(node, "mode", issta? "sta":"ap");
    if (issta) {
        cJSON *sta = cJSON_CreateObject();
        nvs__fetch_str("sta_ssid", temp, sizeof(temp),EXAMPLE_ESP_WIFI_SSID);
        cJSON_AddStringToObject(sta, "ssid", temp);
        nvs__fetch_str("hostname",  temp, sizeof(temp),"interfacez.local");
        cJSON_AddStringToObject(sta, "hostname", temp);
        // only DHCP for now
        cJSON_AddStringToObject(sta, "ip", "dhcp");
        cJSON_AddItemToObject(node, "sta", sta);
    } else {
        cJSON *ap = cJSON_CreateObject();
        nvs__fetch_str("ap_ssid", temp, sizeof(temp),EXAMPLE_ESP_WIFI_SSID);
        cJSON_AddStringToObject(ap, "ssid", temp);
        nvs__fetch_str("hostname",  temp, sizeof(temp),"interfacez.local");
        cJSON_AddStringToObject(ap, "hostname", temp);
        cJSON_AddNumberToObject(ap, "channel", nvs__u8("ap_chan", 3));

        addr.addr = wifi__config_get_ip();
        inet_ntoa_r( addr , temp, sizeof(temp) );
        cJSON_AddStringToObject(ap, "ip", temp);

        addr.addr = wifi__config_get_gw();
        inet_ntoa_r( addr , temp, sizeof(temp) );
        cJSON_AddStringToObject(ap, "gw", temp);

        addr.addr = wifi__config_get_netmask();
        inet_ntoa_r( addr , temp, sizeof(temp) );
        cJSON_AddStringToObject(ap, "netmask", temp);

        cJSON_AddItemToObject(node, "ap", ap);
    }
}


static void wifi__ap_json_entry(void *user, uint8_t auth, uint8_t channel, const char *ssid, size_t ssidlen)
{
    cJSON *e = cJSON_CreateObject();
    cJSON *array = (cJSON *)user;

    cJSON_AddNumberToObject(e, "auth", auth);
    cJSON_AddStringToObject(e, "ssid", ssid);
    cJSON_AddNumberToObject(e, "channel", channel);
    cJSON_AddItemToArray(array, e);
}

static cJSON *scan_array = NULL;

void wifi__ap_json_finished(void *user)
{
    scan_array = (cJSON*)user;
}

cJSON *wifi__ap_get_json()
{
    return scan_array;
}


static const wifi_scan_parser_t json_scan_parser =  {
    .reset = NULL,
    .apcount = NULL,
    .ap = &wifi__ap_json_entry,
    .finish =&wifi__ap_json_finished
};


int wifi__scan_json()
{
    if (scan_array) {
        cJSON_Delete(scan_array);
        scan_array = NULL;
    }

    cJSON *array =  cJSON_CreateArray();

    return wifi__scan(&json_scan_parser, array);
}

#define MAX_AP 16
void wifi__parse_scan()
{
    uint16_t ap_num = MAX_AP;
    wifi_ap_record_t ap_records[MAX_AP];
    // TBD: check if this fits on stack.
    if (esp_wifi_scan_get_ap_records(&ap_num, ap_records)<0) {
        return;
    }

    if (scan_parser==NULL)
        return;

    int required_len = 0; // Number of APs found

    for (int i=0;i<ap_num; i++) {
        // Only allow Open+PSK
        switch ( ap_records[i].authmode ) {
        case WIFI_AUTH_OPEN:
        case WIFI_AUTH_WPA_PSK:
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:
#if 0
        case WIFI_AUTH_WPA3_PSK:
#endif
            break;
        default:
            continue; // Skip AP
        }

        required_len += strlen((char*)ap_records[i].ssid); // Include AP SSID len

        // authmode
        // ssid
    }
    // Fill in AP resource.
    if (scan_parser->reset)
        scan_parser->reset(scan_parser_data);
//    aplist_resource__clear(&aplistresource);
    //    aplist_resource__resize(&aplistresource, required_len);
    if (scan_parser->apcount)
        scan_parser->apcount(scan_parser_data, ap_num, required_len);

    //aplist_resource__setnumaps(&aplistresource, ap_num );
    for (int i=0;i<ap_num; i++) {
        scan_parser->ap(scan_parser_data, ap_records[i].authmode, ap_records[i].primary, (const char*)ap_records[i].ssid, strlen((char*)ap_records[i].ssid));
    }
    if (scan_parser->finish)
        scan_parser->finish(scan_parser_data);

    scan_parser = NULL;
}

int wifi__scan( const wifi_scan_parser_t *parser, void *data )
{
    if (scan_parser!=NULL)
        return -1;

    scan_parser = parser;
    scan_parser_data = data;

    //aplist_resource__clear(&aplistresource);

    int r = wifi__start_scan();
    if (r<0)
        scan_parser = NULL;

    return r;
}

int wifi__config_sta(const char *ssid, const char *pwd)
{
    esp_err_t err;

    err = nvs__set_str("sta_ssid", ssid);

    if (err<0)
        return err;

    if (pwd) {
        err = nvs__set_str("sta_pwd", pwd);
        if (err<0)
            return err;
    }

    err = nvs__set_u8("wifi", WIFI_MODE_STA);

    if (err<0)
        return err;

    err = nvs__commit();

    if (err<0)
        return err;

    wifi__init_wpa2();

    return 0;
}

int wifi__config_ap(const char *ssid, const char *pwd, uint8_t channel)
{
    esp_err_t err;

    if (ssid) {
        err = nvs__set_str("ap_ssid", ssid);

        if (err<0)
            return err;
    }

    if (pwd) {
        err = nvs__set_str("ap_pwd", pwd);

        if (err<0)
            return err;
    }

    if (channel>0) {
        err = nvs__set_u8("ap_chan", channel);

        if (err<0)
            return err;
    }

    err = nvs__set_u8("wifi", WIFI_MODE_AP);

    if (err<0)
        return err;

    err = nvs__commit();

    if (err<0)
        return err;

    wifi__init_softap();

    return 0;

}

int wifi__get_clients()
{
    wifi_sta_list_t list;

    if (esp_wifi_ap_get_sta_list(&list)<0)
        return -1;

    return list.num;
}

int wifi__get_ip_info(uint32_t *addr, uint32_t *netmask, uint32_t *gw)
{
    esp_netif_ip_info_t info;
    int r = esp_netif_get_ip_info(netif, &info);
    if (r<0)
        return -1;
    *addr = info.ip.addr;
    *netmask = info.netmask.addr;
    *gw = info.gw.addr;
    return 0;
}

