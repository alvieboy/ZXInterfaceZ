#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "defs.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "led.h"
#include <string.h>
#include "interfacez_resources.h"

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_SCANNING_BIT BIT1

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


static void ip_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)

{
    ip_event_got_ip_t* event;

    switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
        event = (ip_event_got_ip_t*) event_data;
/*        ESP_LOGI(TAG, "got ip: %s",
 ip4addr_ntoa(&event->ip_info.ip));*/
        ESP_LOGI(TAG, "got ip: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        led__set(LED2, 1);
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

int wifi__start_scan()
{
    aplist_resource__clear(&aplistresource);

    int r = esp_wifi_scan_start(&scan_config, false);
    if (r<0) {
        ESP_LOGE(TAG, "Cannot start scan process");
        return r;
    }
    xEventGroupSetBits(s_wifi_event_group, WIFI_SCANNING_BIT);
    return 0;
}

#define MAX_AP 16
void wifi__parse_scan()
{
    uint16_t ap_num = MAX_AP;
    wifi_ap_record_t ap_records[MAX_AP];
    // TBD: check if this fits on stack.
    if (esp_wifi_scan_get_ap_records(&ap_num, ap_records)<0) {
        return ;
    }
    int required_len = 1; // Number of APs found

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

        required_len += 1; // AP flags
        required_len += strlen((char*)ap_records[i].ssid) +1; // Include AP SSID len

        // authmode
        // ssid
    }
    // Fill in AP resource.
    aplist_resource__clear(&aplistresource);
    aplist_resource__resize(&aplistresource, required_len);
    aplist_resource__setnumaps(&aplistresource, ap_num );
    for (int i=0;i<ap_num; i++) {
        uint8_t flags = 0;
        switch ( ap_records[i].authmode ) {
        case WIFI_AUTH_OPEN:
        case WIFI_AUTH_WPA_PSK:
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:
            flags=1;
            break;
        default:
            break;
        }
        aplist_resource__addap(&aplistresource, flags, (const char*)ap_records[i].ssid,
                              strlen((char*)ap_records[i].ssid));
    }


}

void wifi_init_softap()
{

    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
    IP4_ADDR(&info.ip, 192, 168, 120, 1);
    IP4_ADDR(&info.gw, 0, 0, 0, 0);//192, 168, 120, 1);
    //IP4_ADDR(&info.gw, 192, 168, 120, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .channel = 3,
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s, max sta %d", 
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_MAX_STA_CONN);
}

void wifi_init_wpa2()
{
    issta = true;

    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL));


    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "SotaoDosHorrores",
            .password = "linuxrulez"
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA,
                                          (WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N)
                                         )
                   );

    strcpy(wifi_ssid, (char*)wifi_config.sta.ssid );

    //esp_wifi_set_ps (WIFI_PS_NONE);
    ESP_ERROR_CHECK( esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BW_HT20) );



    ESP_ERROR_CHECK(esp_wifi_start());

}

void wifi__init()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //wifi_init_wpa2();
    wifi_init_softap();
}
