#include "systemevent.h"
#include "log.h"
#include "scsidev.h"
#include "scsi_diskio.h"
#include <string.h>
#include "fileaccess.h"

#define STORAGETAG "STORAGE"

#define MAX_MOUNTED_DEVICES 4

struct {
    char *basepath;
    void *dev;
} mount_devices[MAX_MOUNTED_DEVICES] = {0};


static int storage__register_mount_device(const char *basepath, void *dev)
{
    int i;
    for (i=0;i<MAX_MOUNTED_DEVICES;i++) {
        if (mount_devices[i].basepath==0) {
            mount_devices[i].basepath = strdup(basepath);
            mount_devices[i].dev = dev;
            register_mountpoint(&basepath[1]);
            return 0;
        }
    }
    return -1;
}

static int storage__unregister_mount_device(const char *basepath)
{
    int i;
    for (i=0;i<MAX_MOUNTED_DEVICES;i++) {
        if (strcmp(mount_devices[i].basepath, basepath)==0) {
            free(mount_devices[i].basepath);
            mount_devices[i].basepath = NULL;
            mount_devices[i].dev = 0;
            unregister_mountpoint(&basepath[1]);
            return 0;
        }
    }
    return -1;
}

static const char *storage__get_mountpoint(void *dev)
{
    int i;
    for (i=0;i<MAX_MOUNTED_DEVICES;i++) {
        if (dev == mount_devices[i].dev) {
            return mount_devices[i].basepath;
        }
    }
    return NULL;
}

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
    storage__register_mount_device(name, dev);
#endif
}

void storage__detach_scsidev(scsidev_t *dev)
{
    const char *mp = storage__get_mountpoint(dev);
    if (mp==NULL)
        return;

    vfs_fat_scsi_umount(dev, mp);
    storage__unregister_mount_device(mp);
}

static void storage__handleevent(const systemevent_t *event, void*user)
{
    ESP_LOGI(STORAGETAG, "Storage event 0x%02x ctx=%p", event->event, event->ctxdata);

    switch (event->event) {
    case SYSTEMEVENT_STORAGE_BLOCKDEV_ATTACH:
        storage__attach_scsidev(event->ctxdata);
        break;
    case SYSTEMEVENT_STORAGE_BLOCKDEV_DETACH:
        storage__detach_scsidev(event->ctxdata);
        break;

    default:
        break;
    }
}

void storage__init()
{
    systemevent__register_handler(SYSTEMEVENT_TYPE_STORAGE, storage__handleevent, NULL);
}
