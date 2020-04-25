
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "errno.h"
#include "defs.h"
#include "command.h"
#include "strtoint.h"
#include "netcomms.h"
#include "fpga.h"


#if 0
static int fpga_ota__chunk(command_t *cmdt);

static fpga_program_state_t pgmstate;

int fpga_ota__performota(command_t *cmdt, int argc, char **argv)
{
    int romsize;

    if (argc<1)
        return COMMAND_CLOSE_ERROR;

    // Extract size from params.
    if (strtoint(argv[0], &romsize)<0) {
        return COMMAND_CLOSE_ERROR;
    }
    
    cmdt->romsize = romsize;
    cmdt->romoffset = 0;
    cmdt->rxdatafunc = &fpga_ota__chunk;
    cmdt->state = READDATA;
    cmdt->reported_progress = 0;

    if (fpga__startprogram(&pgmstate)<0) {
        return COMMAND_CLOSE_ERROR;
    }

    return COMMAND_CONTINUE; // Continue receiving data.
}

static int fpga_ota__chunk(command_t *cmdt)
{
    unsigned remain = cmdt->romsize - cmdt->romoffset;
    esp_err_t err;

    if (remain<cmdt->len) {
        // Too much data, complain
        ESP_LOGE(TAG, "FPGA OTA: expected max %d but got %d bytes", remain, cmdt->len);
        return COMMAND_CLOSE_ERROR;
    }

    // Image header checked
    err = fpga__program(&pgmstate, (const void *)cmdt->rx_buffer, cmdt->len);

    if (err!=0)
        return -1;

    cmdt->romoffset += cmdt->len;
    cmdt->len = 0; // Reset receive ptr.
    remain = cmdt->romsize - cmdt->romoffset;

    ESP_LOGI(TAG, "FPGA OTA: remain size %d", remain);

    /* Report progress if needed */
    int current_progress = (cmdt->romoffset*100) / cmdt->romsize;
    if (cmdt->reported_progress != current_progress) {
        cmdt->reported_progress =current_progress;
        netcomms__send_progress(cmdt->socket, 100, current_progress);
    }


    if (remain==0) {
        if (fpga__finishprogram(&pgmstate)<0)
        {
            return COMMAND_CLOSE_ERROR;
        }
        ESP_LOGI(TAG, "FPGA OTA: finished");
        return COMMAND_CLOSE_OK;
    }

    return COMMAND_CONTINUE;
}

#else
int fpga_ota__performota(command_t *cmdt, int argc, char **argv)
{
    return -1;
}
#endif
