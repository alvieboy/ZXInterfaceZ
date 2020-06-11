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

#ifndef __linux__

//#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

//static char ota_write_data[BUFFSIZE + 1] = { 0 };

static const esp_partition_t *configured;
static const esp_partition_t *running;
static const esp_partition_t *update_partition = NULL;

static esp_ota_handle_t update_handle;
static bool image_header_was_checked;


static int ota__chunk(command_t *cmdt);


int ota__performota(command_t *cmdt, int argc, char **argv)
{
    int romsize;

    if (argc<1)
        return COMMAND_CLOSE_ERROR;

    // Extract size from params.
    if (strtoint(argv[0], &romsize)<0) {
        return COMMAND_CLOSE_ERROR;
    }
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    update_handle = 0;

    update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA example");

    configured = esp_ota_get_boot_partition();
    running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);


    update_partition = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    assert(update_partition != NULL);

    cmdt->romsize = romsize;
    cmdt->romoffset = 0;
    cmdt->rxdatafunc = &ota__chunk;
    cmdt->state = READDATA;
    cmdt->reported_progress = 0;

    image_header_was_checked = false;

    return COMMAND_CONTINUE; // Continue receiving data.
}



int ota__validimage(const uint8_t *data)
{

    esp_app_desc_t new_app_info;

    memcpy(&new_app_info, &data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)],
           sizeof(esp_app_desc_t));

    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
    esp_app_desc_t invalid_app_info;
    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
    }

    // check current version with last invalid partition
    if (last_invalid_app != NULL) {
        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
            ESP_LOGW(TAG, "New version is the same as invalid version.");
            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
            return -1;
        }
    }
    return 0;
}

static int ota__chunk(command_t *cmdt)
{
    unsigned remain = cmdt->romsize - cmdt->romoffset;
    esp_err_t err;

    if (remain<cmdt->len) {
        // Too much data, complain
        ESP_LOGE(TAG, "ROM: expected max %d but got %d bytes", remain, cmdt->len);
        return COMMAND_CLOSE_ERROR;
    }

    if (image_header_was_checked == false) {

        if ( cmdt->len > ( sizeof(esp_image_header_t) +
                          sizeof(esp_image_segment_header_t) +
                          sizeof(esp_app_desc_t) ) ) {
            /* Check header */
            if (ota__validimage(cmdt->rx_buffer)!=0) {
                ESP_LOGI(TAG, "OTA: Valid image detected");
                return COMMAND_CLOSE_ERROR; // Abort
            }

            err = esp_ota_begin(update_partition, cmdt->romsize, &update_handle);

            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                return COMMAND_CLOSE_ERROR;
            }
            ESP_LOGI(TAG, "OTA: update handle %d", update_handle);
            image_header_was_checked = true;
        } else {
            // Need more data.
            return COMMAND_CONTINUE;
        }
    }

    if (image_header_was_checked) {
        // Image header checked
        err = esp_ota_write( update_handle, (const void *)cmdt->rx_buffer, cmdt->len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA: Error writing OTA image");
            return COMMAND_CLOSE_ERROR;
        }
    }

    cmdt->romoffset += cmdt->len;
    cmdt->len = 0; // Reset receive ptr.
    remain = cmdt->romsize - cmdt->romoffset;

    ESP_LOGI(TAG, "OTA: remain size %d", remain);

    /* Report progress if needed */
    int current_progress = (cmdt->romoffset*100) / cmdt->romsize;
    if (cmdt->reported_progress != current_progress) {
        cmdt->reported_progress =current_progress;
        netcomms__send_progress(cmdt->socket, 100, current_progress);
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
}


#else
int ota__performota(command_t *cmdt, int argc, char **argv)
{
    return -1;
}
#endif
