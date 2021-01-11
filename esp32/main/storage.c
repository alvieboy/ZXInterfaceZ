#include "systemevent.h"
#include "log.h"
#include "scsidev.h"
#include "scsi_diskio.h"

#define STORAGETAG "STORAGE"

void storage__attach_scsidev(scsidev_t *dev)
{
    char name[17];
    name[0] =  '/';
    scsidev__getname(dev, &name[1]);
#ifndef __linux__
    ESP_LOGI(STORAGETAG, "Attempting to register '%s'", name);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    esp_err_t r = vfs_fat_scsi_mount(name,
                                     dev,
                                     &mount_config);
    if (r!=ESP_OK) {
        ESP_LOGE(STORAGETAG,"Cannot register filesystem");
    }
#endif
}

static void storage__handleevent(const systemevent_t *event, void*user)
{
    ESP_LOGI(STORAGETAG, "Storage event 0x%02x ctx=%p", event->event, event->ctxdata);

    switch (event->event) {
    case SYSTEMEVENT_STORAGE_BLOCKDEV_ATTACH:
        storage__attach_scsidev(event->ctxdata);
        break;
    default:
        break;
    }
}

void storage__init()
{
    systemevent__register_handler(SYSTEMEVENT_TYPE_STORAGE, storage__handleevent, NULL);
}
