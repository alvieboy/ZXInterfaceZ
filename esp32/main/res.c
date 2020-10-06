#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "errno.h"
#include "defs.h"
#include "command.h"
#include "strtoint.h"
#include "sna_relocs.h"
#include "fpga.h"
#include "res.h"

static int res__chunk(command_t *cmdt);

int res__upload(command_t *cmdt, int argc, char **argv)
{
    int romsize;

    if (argc<1)
        return COMMAND_CLOSE_ERROR;

    // Extract size from params.
    if (strtoint(argv[0], &romsize)<0) {
        return COMMAND_CLOSE_ERROR;
    }
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */

    ESP_LOGI(TAG, "Starting generic resource upload %d", romsize);

    fpga__set_trigger(FPGA_FLAG_TRIG_RESOURCEFIFO_RESET);

    cmdt->romsize = romsize;
    cmdt->romoffset = 0;
    cmdt->rxdatafunc = &res__chunk;
    cmdt->state = READDATA;

    return COMMAND_CONTINUE; // Continue receiving data.
}

static int res__chunk(command_t *cmdt)
{
    unsigned remain = cmdt->romsize - cmdt->romoffset;

    if (remain<cmdt->len) {
        // Too much data, complain
        ESP_LOGE(TAG, "RES: expected max %d but got %d bytes", remain, cmdt->len);
        return COMMAND_CLOSE_ERROR;
    }

 
    fpga__load_resource_fifo(cmdt->rx_buffer, cmdt->len, 100);

    cmdt->romoffset += cmdt->len;
    cmdt->len = 0; // Reset receive ptr.
    remain = cmdt->romsize - cmdt->romoffset;

    ESP_LOGI(TAG, "RES: remain size %d", remain);

    if (remain==0) {
        return COMMAND_CLOSE_OK;
    }


    return COMMAND_CONTINUE;
}


