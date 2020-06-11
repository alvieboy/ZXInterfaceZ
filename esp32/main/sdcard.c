#include "esp_system.h"
#include "esp_log.h"
#include "defs.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include <sys/types.h>
#include <dirent.h>

#ifndef __linux__

void sdcard__init()
{
    ESP_LOGI(TAG, "Using SDMMC peripheral");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#ifdef SDMMC_PIN_DET
    slot_config.gpio_cd = SDMMC_PIN_DET;
#endif
    // To use 1-line SD mode, uncomment the following line:
    // slot_config.width = 1;

    gpio_set_pull_mode(SDMMC_PIN_CMD, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(SDMMC_PIN_D0, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(SDMMC_PIN_D1, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(SDMMC_PIN_D2, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(SDMMC_PIN_D3, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes


    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "SDMMC card mounted");
    {
        DIR *d;
        d = opendir("/sdcard");
        if (!d) {
            ESP_LOGI(TAG, "Cannot open directory");
        } else {
            struct dirent *e;
            while ((e=readdir(d))) {
                ESP_LOGI(TAG, " * %s", e->d_name);
            }
            ESP_LOGI(TAG, "<< EOD");
            closedir(d);
        }
    }
}

bool sdcard__isconnected(void)
{
    return false;
}
#else
bool sdcard__isconnected(void)
{
    return false;
}

void sdcard__init()
{
}

#endif
