
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/event_groups.h"
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
#include "systemevent.h"

#define TAG "WIFI"


#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

#define DEFAULT_TXPOWER 20 /* +5dB */

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

static volatile wifi_status_t wifi_status;
static volatile uint8_t wifi_status_bits;

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

static bool wifi__isvalidchannel(int channel)
{
    unsigned i;
    for (i=0; i<wifi_channels.num_chans;i++) {
        if (wifi_channels.info[i].chan == channel)
            return true;
    }
    return false;
}

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

static void wifi__set_status(wifi_status_t status)
{
    wifi_status = status;
    systemevent__send(SYSTEMEVENT_TYPE_WIFI, SYSTEMEVENT_WIFI_STATUS_CHANGED);
}

static void wifi__emit_network_changed()
{
    systemevent__send(SYSTEMEVENT_TYPE_NETWORK, SYSTEMEVENT_NETWORK_STATUS_CHANGED);
}

static void wifi__emit_scan_finished()
{
    systemevent__send(SYSTEMEVENT_TYPE_WIFI, SYSTEMEVENT_WIFI_SCAN_COMPLETED);
}

wifi_status_t wifi__get_status()
{
    if (wifi__scanning())
        return WIFI_SCANNING;
    return wifi_status;
}

static const char *status_strings[] = {
    "Scanning",
    "Disconnected",
    "Accepting",
    "Connecting",
    "Obtaining IP",
    "Connected"
};


const char *wifi__status_string(wifi_status_t status)
{
    unsigned idx =(unsigned)status;
    if (idx<sizeof(status_strings)/sizeof(status_strings[0])) {
        return status_strings[idx];
    }
    return "Unknown";
}

int wifi__get_sta_ssid(char *dest, unsigned size)
{
    return nvs__fetch_str("sta_ssid", dest, size, "");
}

static void wifi__init_core(void);

static bool issta = false;
char wifi_ssid[33];

bool wifi__isconnected()
{
    //return xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT;
    return wifi_status_bits & WIFI_CONNECTED_BIT;
}

bool wifi__scanning()
{
    //    return xEventGroupGetBits(s_wifi_event_group) & WIFI_SCANNING_BIT;
    return wifi_status_bits & WIFI_SCANNING_BIT;
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

    ESP_LOGI(TAG,"Installing MDNS service");

    ESP_ERROR_CHECK(mdns_init());
    nvs__str("hostname",hostname,sizeof(hostname),"interfacez.local");
    ESP_ERROR_CHECK(mdns_instance_name_set("ZX InterfaceZ"));

    ESP_ERROR_CHECK(mdns_hostname_set("interfacez"));

    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    mdns_service_instance_name_set("_http", "_tcp", "ZX InterfaceZ Web Interface");

    mdns_service_add(NULL, "_zxictrl", "_tcp", BUFFER_PORT, NULL, 0);
    mdns_service_instance_name_set("_zxictrl", "_tcp", "ZX InterfaceZ Control Interface");
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)

{
    ip_event_got_ip_t* event;

    if (event_base!=IP_EVENT)
        return;

    switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
        event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        //xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_status_bits |= WIFI_CONNECTED_BIT;
        led__set(LED2, 1);
#if 1
        wifi__set_status(WIFI_CONNECTED);
#endif
        wifi__emit_network_changed();

        //setup_mdns();

        break;
    default:
        ESP_LOGW(TAG,"Unhandled IP event %d", event_id);
        break;
    }
}

static void wifi__parse_scan(void);

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base != WIFI_EVENT)
        return;

    switch (event_id) {

    case WIFI_EVENT_STA_START:
        wifi__set_status(WIFI_CONNECTING);
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        led__set(LED2, 0);
        //xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_status_bits &= ~WIFI_CONNECTED_BIT;
        wifi__set_status(WIFI_DISCONNECTED);
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_STOP:
        wifi__set_status(WIFI_DISCONNECTED);
        break;

    case WIFI_EVENT_STA_CONNECTED:
        wifi__set_status(WIFI_WAIT_IP_ADDRESS);
        break;
#if 0
    case WIFI_EVENT_STA_GOT_IP:
        wifi__set_status(WIFI_CONNECTED);
        break;
#endif
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
        //xEventGroupClearBits(s_wifi_event_group, WIFI_SCANNING_BIT);
        wifi_status_bits &= ~WIFI_SCANNING_BIT;
        wifi__emit_scan_finished();

        //case WIFI_EVENT_STA_CONNECTED:
        break;
    case WIFI_EVENT_AP_START:
        ESP_LOGI(TAG,"WiFi AP started");
        wifi__set_status(WIFI_ACCEPTING);
        //setup_mdns();
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

static void wifi__teardown()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    //mdns_free();
    esp_wifi_deinit();
}

static void wifi__init_softap()
{
    wifi__teardown();

    if (netif) {
        esp_netif_destroy(netif);
    }

    netif = esp_netif_create_default_wifi_ap();

    wifi__init_core();

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));

    esp_netif_ip_info_t info;
    memset(&info, 0, sizeof(info));

    issta = false;

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

    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(nvs__u8("wifitxpower",DEFAULT_TXPOWER)));

    ESP_LOGI(TAG, "Configured WiFi in AP mode");
    ESP_LOGI(TAG, " SSID    : %s", wifi_config.ap.ssid);
    // ESP_LOGI(TAG, " Password: %s", wifi_config.ap.password);
    ESP_LOGI(TAG, " Channel : %d", wifi_config.ap.channel);
    ESP_LOGI(TAG, " Max conn: %d", wifi_config.ap.max_connection);
}

static void wifi__init_core()
{
#ifndef __linux__ /* we still don't support this */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
#endif

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL));

    wifi_status = WIFI_DISCONNECTED;
}

static void wifi__init_wpa2()
{
    wifi__teardown();

    if (netif) {
        esp_netif_destroy(netif);
    }


    issta = true;

    netif = esp_netif_create_default_wifi_sta();

    wifi__init_core();
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
    //  s_wifi_event_group = xEventGroupCreate();
    wifi_status_bits = 0;

    esp_netif_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi__init_core();

    setup_mdns();

    if (nvs__u8("wifi", WIFI_MODE_AP)==WIFI_MODE_STA) {
        wifi__init_wpa2();
    } else if (nvs__u8("wifi", WIFI_MODE_AP)==WIFI_MODE_AP) {
        wifi__init_softap();
    }

}

int wifi__start_scan()
{
    esp_wifi_disconnect();
    int r = esp_wifi_scan_start(&scan_config, false);
    if (r<0) {
        ESP_LOGE(TAG, "Cannot start scan process: %s", esp_err_to_name(r));
        return r;
    }
    //xEventGroupSetBits(s_wifi_event_group, WIFI_SCANNING_BIT);
    wifi_status_bits |= WIFI_SCANNING_BIT;
    return 0;
}

int wifi__get_rssi()
{
    wifi_ap_record_t wifidata;
    if (esp_wifi_sta_get_ap_info(&wifidata)==0){
        return wifidata.rssi;
    }
    return -1;
}

static esp_err_t wifi__stop()
{
    wifi__teardown();
    nvs__set_u8("wifi", WIFI_MODE_NULL);
    return ESP_OK;
}


void wifi__get_conf_json(cJSON *node)
{
    char temp[64];
    struct ip4_addr addr, mask, gw;
    uint8_t mac[6];

    bool issta = nvs__u8("wifi", WIFI_MODE_AP)==WIFI_MODE_STA;
    cJSON_AddStringToObject(node, "mode", issta? "sta":"ap");
    if (issta) {
        cJSON *sta = cJSON_CreateObject();
        nvs__fetch_str("sta_ssid", temp, sizeof(temp),EXAMPLE_ESP_WIFI_SSID);
        cJSON_AddStringToObject(sta, "ssid", temp);
        nvs__fetch_str("hostname",  temp, sizeof(temp),"interfacez.local");
        cJSON_AddStringToObject(sta, "hostname", temp);
        // only DHCP for now
        cJSON_AddStringToObject(sta, "ip_config", "dhcp");
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
    // Add status node

    if (netif!=NULL) {
        cJSON *status = cJSON_CreateObject();

        esp_netif_get_mac(netif, &mac[0]);

        sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
                mac[0],
                mac[1],
                mac[2],
                mac[3],
                mac[4],
                mac[0]);

        cJSON_AddStringToObject(status, "connected", wifi__isconnected()?"true":"false");

        cJSON_AddStringToObject(status, "mac", temp);

        wifi__get_ip_info(&addr.addr, &mask.addr, &gw.addr);

        inet_ntoa_r( addr , temp, sizeof(temp) );
        cJSON_AddStringToObject(status, "ip", temp);
        inet_ntoa_r( gw , temp, sizeof(temp) );
        cJSON_AddStringToObject(status, "gw", temp);
        inet_ntoa_r( mask , temp, sizeof(temp) );
        cJSON_AddStringToObject(status, "netmask", temp);

        cJSON_AddItemToObject(node, "status", status);
    }
}

static esp_err_t wifi__fetch_json_ip_mask_gw(cJSON *node, const char **errstr,
                                             ip4_addr_t *ip,
                                             ip4_addr_t *netmask,
                                             ip4_addr_t *gw)
{
    esp_err_t r;

    if (ip) {
        // fetch IP address
        r = json__get_ip(node,"ip", ip);
        if (r<0) {
            *errstr = "Invalid IP address";
            return r;
        }
    }
    if (netmask) {
        r = json__get_ip(node,"netmask", netmask);
        if (r<0) {
            *errstr = "Invalid IP netmask";
            return r;
        }
    }
    if (gw) {
        r = json__get_ip(node,"gw", gw);
        if (r<0) {
            *errstr = "Invalid IP gateway";
            return r;
        }
    }
    // TBD: validate IP/Netmask/GW

    return ESP_OK;
}

static esp_err_t wifi__fetch_json_ssid_passsword(cJSON *node, const char **errstr,
                                                 const char **ssid,
                                                 const char **password)
{
    *ssid = json__get_string(node, "ssid");
    *password = json__get_string(node, "password");

    return ESP_OK;
}

static esp_err_t wifi__set_conf_json_ap(cJSON *node, const char **errstr)
{
    int r;

    ip4_addr_t ip, mask;
    const char *ssid, *password;
    int chan = -1;

    r = wifi__fetch_json_ssid_passsword(node, errstr, &ssid, &password);

    if (r<0)
        return r;

    r = wifi__fetch_json_ip_mask_gw(node, errstr,&ip,&mask,NULL);

    cJSON *n = cJSON_GetObjectItemCaseSensitive(node, "channel");
    if ((!n) || cJSON_IsNumber(n)) {
        *errstr = "Invalid channel";
        r = -1;
    } else {
        chan = n->valueint;
        if (!wifi__isvalidchannel(chan)) {
            *errstr = "Invalid channel";
            r = -1;
        }
    }
    if (r<0)
        return r;

    uint32_t prev_ip = wifi__config_get_ip();
    uint32_t prev_gw = wifi__config_get_gw();
    uint32_t prev_mask = wifi__config_get_netmask();

    nvs__set_u32("ip", ip.addr);
    nvs__set_u32("mask", mask.addr);
    nvs__set_u32("gw", 0);
    

    r = wifi__config_ap(ssid, password, chan);

    if (r<0) {

        nvs__set_u32("ip", prev_ip);
        nvs__set_u32("mask", prev_mask);
        nvs__set_u32("gw", prev_gw);

        *errstr = "Cannot apply configuration";
    }


    return r;
}

static esp_err_t wifi__set_conf_json_sta(cJSON *node, const char **errstr)
{
    int r = ESP_FAIL;
    ip4_addr_t ip, mask, gw;

    const char *ssid, *password;

    r = wifi__fetch_json_ssid_passsword(node, errstr, &ssid, &password);

    if (r<0)
        return r;

    const char *inet = json__get_string(node, "inet");

    if (!inet) {
        return r;
    } else if (strcmp(inet,"static")==0) {
#if 0
        r = wifi__fetch_json_ip_mask_gw(node, errstr,&ip,&mask,&gw);
        if (r<0)
            return r;
#endif
        r = -1;
        *errstr = "Static IP currently unsupported";

    } else if (strcmp(inet,"dhcp")==0) {
        r = ESP_OK;
    } else {
        *errstr = "Invalid inet settings";
        r = -1;
    }

    if (r==ESP_OK) {
        r = wifi__config_sta(ssid, password);
        if (r<0) {
            *errstr = "Cannot apply configuration";
        }
    }


    return r;
}

esp_err_t wifi__set_conf_json(cJSON *node, const char **errstr)
{
    esp_err_t r = ESP_FAIL;
    const char *mode = cJSON_GetObjectItem(node, "mode")->valuestring;

    if (strstr(mode,"disabled")==0) {
        r = wifi__stop();
    } else if (strstr(mode,"ap")==0) {
        r = wifi__set_conf_json_ap(node, errstr);
    } else if (strstr(mode,"sta")==0) {
        r = wifi__set_conf_json_sta(node, errstr);
    } else {
        *errstr = "Invalid mode setting";
    }

    return r;
}


static void wifi__ap_json_entry(void *user, uint8_t auth, uint8_t channel, int8_t rssi, const char *ssid, size_t ssidlen)
{
    cJSON *e = cJSON_CreateObject();
    cJSON *array = (cJSON *)user;

    cJSON_AddNumberToObject(e, "auth", auth);
    cJSON_AddStringToObject(e, "ssid", ssid);
    cJSON_AddNumberToObject(e, "channel", channel);
    cJSON_AddNumberToObject(e, "rssi", rssi);
    cJSON_AddItemToArray(array, e);
}


static cJSON *scan_array = NULL;

static void wifi__ap_json_finished(void *user)
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
    wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t)*MAX_AP);
    if (!ap_records) {
        ESP_LOGE(TAG,"Cannot allocate memory for AP records");
        return;
    }

    if (esp_wifi_scan_get_ap_records(&ap_num, ap_records)<0) {
        free(ap_records);
        return;
    }

    if (scan_parser==NULL) {
        free(ap_records);
        return;
    }

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

    if (scan_parser->apcount)
        scan_parser->apcount(scan_parser_data, ap_num, required_len);

    for (int i=0;i<ap_num; i++) {
        scan_parser->ap(scan_parser_data,
                        ap_records[i].authmode,
                        ap_records[i].primary,
                        ap_records[i].rssi,
                        (const char*)ap_records[i].ssid,
                        strlen((char*)ap_records[i].ssid)
                       );
    }

    if (scan_parser->finish)
        scan_parser->finish(scan_parser_data);

    scan_parser = NULL;

    free(ap_records);
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

int wifi__set_mode(wifi_mode_t mode)
{
    switch (mode) {
    case WIFI_MODE_AP:
        nvs__set_u8("wifi", WIFI_MODE_AP);
        nvs__commit();
        wifi__init_softap();
        break;
    case WIFI_MODE_STA:
        nvs__set_u8("wifi", WIFI_MODE_STA);
        nvs__commit();
        wifi__init_wpa2();
        break;
    default:
        break;
    }

    return 0;
}
