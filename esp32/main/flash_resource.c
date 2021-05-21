#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "defs.h"
#include "resource.h"
#include "flash_resource.h"
#include "esp_spiffs.h"
#include "fpga.h"
#include "fileaccess.h"
#include "version.h"

#define TAG "FLASH"

static bool fallback = false;

int flash_resource__deinit(void)
{
#ifndef __linux__
    return esp_vfs_spiffs_unregister("resources");
#else
    return 0;
#endif
}

bool flash_resource__is_fallback(void)
{
    return fallback;
}

static int flash_resource__mount_partition(const char *pname)
{
#ifndef __linux__
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = pname,
      .max_files = 128,
      .format_if_mount_failed = false
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return -1;
    }
    return 0;
#endif
    return 0;
}

static bool flash_resource__version_match()
{
    char fversion[256];
    int fd = __open("/spiffs/VERSION", O_RDONLY);
    if (fd<0)
        return -1;

    int r = __read(fd, fversion, sizeof(fversion)-1);

    close(fd);

    if (r<0) {
        return -1;
    }
    fversion[r] = '\0';
    if (strcmp(fversion, gitversion)==0)
        return 0;

    return -1;

}

int flash_resource__init(void)
{
    if (flash_resource__mount_partition("resources")==0) {
        if (flash_resource__version_match()==0)  {
            return 0;
        }
        ESP_LOGE(TAG,"Version mismatch on resources partition");
        // version problem, umount
#ifndef __linux__
        esp_vfs_spiffs_unregister("/spiffs");
#endif
    }
    // Use fallback
    fallback = true;
    ESP_LOGE(TAG,"Using fallback resources partition");
    return flash_resource__mount_partition("fallback");
}

const char *flash_resource__get_root()
{/*
    if (fallback) {
        return "/fallback";
    }*/
    return "/spiffs";
}


#if 0
uint8_t flash_resource__type(struct resource *r)
{
    struct flash_resource *fr = (struct flash_resource*)r;
    return fr->data->type;
}

uint16_t flash_resource__len(struct resource *r)
{
    struct flash_resource *fr = (struct flash_resource*)r;
    ESP_LOGI(TAG,"Flash resource size: %d", fr->data->len);
    return fr->data->len;
}

int flash_resource__sendToFifo(struct resource *r)
{
    struct flash_resource *fr = (struct flash_resource*)r;
    return fpga__load_resource_fifo( fr->data->data, fr->data->len, RESOURCE_DEFAULT_TIMEOUT);
}

static struct flash_resource flashresource = {
    .r = {
        .type = &flash_resource__type,
        .len = &flash_resource__len,
        .sendToFifo = &flash_resource__sendToFifo,
        .update = NULL,
    },
    .data = NULL
};

struct flash_resource *flash_resource__find(uint8_t id)
{
    const struct flashresourcedata *data = flashresourcedata__finddata(id);
    if (data==NULL) {
        return NULL;
    }
    flashresource.data = data;
    return &flashresource;
}
#endif
