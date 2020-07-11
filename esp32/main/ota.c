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
#include "ota.h"

#define HASH_LEN 32 /* SHA-256 digest length */

int ota__init(ota_stream_handle_t *h)
{
    h->size = 0;
    h->offset = 0;
    h->update_partition = NULL;
    h->update_handle = 0;
    h->image_header_was_checked = false;
    return 0;
}

static int ota__chunk_cmd(command_t *cmdt);

int ota__performota_cmd(command_t *cmdt, int argc, char **argv)
{
    int romsize;
    ota_stream_handle_t h;

    if (argc<1)
        return COMMAND_CLOSE_ERROR;

    // Extract size from params.
    if (strtoint(argv[0], &romsize)<0) {
        return COMMAND_CLOSE_ERROR;
    }
    ota__init(&h);
    if (ota__start(&h, romsize)<0)
        return -1;

    cmdt->romsize = romsize;
    cmdt->romoffset = 0;
    cmdt->rxdatafunc = &ota__chunk_cmd;
    cmdt->state = READDATA;
    cmdt->reported_progress = 0;

    return COMMAND_CONTINUE; // Continue receiving data.

}

int ota__start(ota_stream_handle_t *h, int size)
{
    ESP_LOGI(TAG, "Starting OTA");

    h->size = size;
    h->configured = esp_ota_get_boot_partition();
    h->running = esp_ota_get_running_partition();

    if (h->configured != h->running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 h->configured->address, h->running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }

    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             h->running->type, h->running->subtype, h->running->address);


    h->update_partition = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             h->update_partition->subtype, h->update_partition->address);

    assert(h->update_partition != NULL);

    return esp_ota_begin(h->update_partition, h->size, &h->update_handle);
}



static int ota__chunk_cmd(command_t *cmdt)
{
#if 0
    unsigned remain = cmdt->romsize - cmdt->romoffset;
    esp_err_t err;

    if (remain<cmdt->len) {
        // Too much data, complain
        ESP_LOGE(TAG, "ROM: expected max %d but got %d bytes", remain, cmdt->len);
        return COMMAND_CLOSE_ERROR;
    }

    err = esp_ota_begin(update_partition, cmdt->romsize, &update_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        return COMMAND_CLOSE_ERROR;
    }

    ESP_LOGI(TAG, "OTA: update handle %d", update_handle);

    image_header_was_checked = true;
    
    err = esp_ota_write( update_handle, (const void *)cmdt->rx_buffer, cmdt->len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA: Error writing OTA image");
        return COMMAND_CLOSE_ERROR;
    }


    if (remain==0) {
        ESP_LOGI(TAG, "OTA: finished");
        err = esp_ota_end(update_handle);
        if (err != ESP_OK) {
            if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            return COMMAND_CLOSE_ERROR;
        }

        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            return COMMAND_CLOSE_ERROR;
        }

        ESP_LOGI(TAG, "Prepare to restart system!");

        request_restart();

        return COMMAND_CLOSE_OK;
    }

    return COMMAND_CONTINUE;
#else
    return COMMAND_CLOSE_ERROR;
#endif

}


int ota__chunk(ota_stream_handle_t *h, const uint8_t *data, int len)
{
    unsigned remain = h->size - h->offset;
    esp_err_t err;

    if (remain < len) {
        // Too much data, complain
        ESP_LOGE(TAG, "ROM: expected max %d but got %d bytes", remain, len);
        return -1;
    }


    err = esp_ota_write( h->update_handle, (const void *)data, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA: Error writing OTA image");
        return err;
    }

    remain -= len;
    h->offset += len;

    if (remain==0) {
        ESP_LOGI(TAG, "OTA: finished");
        err = esp_ota_end(h->update_handle);

        if (err != ESP_OK) {
            if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            return -1;
        }

        err = esp_ota_set_boot_partition(h->update_partition);

        return err;
    }
    return 1; // In progress
}

