#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "fpga.h"
#include "esp_system.h"
#include "esp_log.h"
#include "defs.h"
#include "tapplayer.h"

extern const unsigned int tapfile_len;
extern const unsigned char tapfile[];

static void tapplayer__task(void*data)
{
    ESP_LOGI(TAG, "Starting TAP file play len %d", tapfile_len);

    fpga__set_flags(FPGA_FLAG_TAPFIFO_RESET| FPGA_FLAG_TAP_ENABLED);
    fpga__clear_flags(FPGA_FLAG_TAPFIFO_RESET);

    int len = tapfile_len;
    const uint8_t *tap = &tapfile[0];
    int chunk;

    do {
        if (len>0) {
            chunk = fpga__load_tap_fifo(tap, len, 1000);
            //ESP_LOGI(TAG, "TAP: Loaded %d bytes", chunk);
            if (chunk<0){
                ESP_LOGW(TAG, "Cannot write to TAP fifo");
            } else {
                len-=chunk;
                tap+=chunk;
                if (len==0) {
                    ESP_LOGI(TAG,"TAP play finished");
                }
            }
        } 
        vTaskDelay(100 / portTICK_RATE_MS);
    } while (1);
}


void tapplayer__init()
{
    xTaskCreate(tapplayer__task, "tapplayer_task", 4096, NULL, 10, NULL);
}
