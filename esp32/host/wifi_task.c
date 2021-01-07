#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "wifi_task.h"
#include <string.h>

static xQueueHandle scan_queue = NULL;

#define WIFI_CMD_SCAN    1
#define WIFI_CMD_APMODE  2
#define WIFI_CMD_CONNECT 3

void wifi__end_scan()
{
    esp_event_send_internal(WIFI_EVENT,
                            WIFI_EVENT_SCAN_DONE,
                            NULL,
                            0,
                            1000);
}

int do_hw_wifi_scan(void)
{
    printf("WIFI: request scan\n");
    uint32_t val = WIFI_CMD_SCAN;
    xQueueSend(scan_queue, &val, 1000);
    return 0;
}

void wifi_task_apmode(void)
{
    printf("WIFI: request AP mode\n");
    uint32_t val = WIFI_CMD_APMODE;
    xQueueSend(scan_queue, &val, 1000);
}

void wifi_task_connect(void)
{
    printf("WIFI: request connect STA mode\n");
    uint32_t val = WIFI_CMD_CONNECT;
    xQueueSend(scan_queue, &val, 1000);
}

static ip_event_got_ip_t ip_event;

void wifi_task(void *data)
{
    uint32_t resp;
    printf("Waiting for scan requests\n");
    while (1) {
        if (xQueueReceive(scan_queue, &resp, portMAX_DELAY)==pdTRUE) {
            printf("Wifi request received\n");
            switch (resp) {
            case WIFI_CMD_SCAN:
                printf("Requested scan\n");
                vTaskDelay(5000 / portTICK_RATE_MS);
                // Notify completion
                wifi__end_scan();
                break;
            case WIFI_CMD_APMODE:
                vTaskDelay(200 / portTICK_RATE_MS);
                esp_event_send_internal(WIFI_EVENT,
                                        WIFI_EVENT_AP_START,
                                        NULL,
                                        0,
                                        1000);
                break;
            case WIFI_CMD_CONNECT:
                vTaskDelay(200 / portTICK_RATE_MS);
                esp_event_send_internal(WIFI_EVENT, WIFI_EVENT_STA_START, NULL, 0, 1000);
                vTaskDelay(1000 / portTICK_RATE_MS);
                esp_event_send_internal(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, NULL, 0, 1000);
                vTaskDelay(1000 / portTICK_RATE_MS);
                ip_event.ip_info.ip.addr = 0x0a0a0a0a;
                esp_event_send_internal(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event, sizeof(ip_event), 1000);

                break;
            }
        }
    }
}


void wifi_task_init()
{
    scan_queue = xQueueCreate(4, sizeof(uint32_t));
}

