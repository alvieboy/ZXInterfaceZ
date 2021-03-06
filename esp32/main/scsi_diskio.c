#include "scsidev.h"
#include "diskio_impl.h"
#include "diskio.h"
#include "esp_vfs_fat.h"
#include "ffconf.h"
#include "scsidev.h"
#include <string.h>
#include "log.h"

#define TAG "SCSIDEVFF"

static scsidev_t* scsi_devices[FF_VOLUMES] = { NULL };

static void ff_diskio_register_scsidev(BYTE pdrv, scsidev_t* dev);

static BYTE ff_diskio_get_pdrv_card(const scsidev_t *d)
{
    int i;
    for (i=0;i<FF_VOLUMES;i++) {
        if (scsi_devices[i]==d)
            return i;
    }
    return 0xFF;
}

static DSTATUS ff_scsidev_initialize(unsigned char pdrv)
{
    ESP_LOGI(TAG,"initialise");
    //scsidev_t *self = scsi_devices[pdrv];
    return 0;
}

static DSTATUS ff_scsidev_status(unsigned char pdrv)
{
    scsidev_t *self = scsi_devices[pdrv];
    if (self==NULL)
        return -1;
    return 0;
}

static DSTATUS ff_scsi_status_to_dresult(uint8_t scsi_status)
{
    return scsi_status;
}

static DRESULT ff_scsidev_read(unsigned char pdrv, unsigned char* buff, uint32_t sector, unsigned count)
{
    scsidev_t *self = scsi_devices[pdrv];
    uint8_t status;
    ESP_LOGI(TAG,"read sector=%d count=%d", sector, count);
    if (self==NULL)
        return -1;

    int r = scsidev__read(self, buff, sector, count, &status);

    if (r<0)
        return r;

    return ff_scsi_status_to_dresult(status);
}

static DRESULT ff_scsidev_write(unsigned char pdrv, const unsigned char* buff, uint32_t sector, unsigned count)
{
    scsidev_t *self = scsi_devices[pdrv];
    uint8_t status;
    ESP_LOGI(TAG,"write");

    if (self==NULL)
        return -1;
    int r = scsidev__write(self, buff, sector, count, &status);
    if (r<0)
        return r;
    return ff_scsi_status_to_dresult(status);
}

static DRESULT ff_scsidev_ioctl(unsigned char pdrv, unsigned char cmd, void* buff)
{
    scsidev_t *self = scsi_devices[pdrv];
    if (self==NULL)
        return -1;

    ESP_LOGI(TAG, "IOCTL SCSI device 0x%02x %p", pdrv, self);

    ESP_LOGI(TAG,"IOCTL 0x%02x", cmd);

    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
        break;
    case GET_SECTOR_COUNT:
        ESP_LOGI(TAG,"Get sector count: %u", self->sector_count);

        *((DWORD*) buff) = self->sector_count;
        return RES_OK;
    case GET_SECTOR_SIZE:
        ESP_LOGI(TAG,"Get sector size: %u", self->sector_size);
        *((WORD*) buff) = self->sector_size;
        return RES_OK;
    case GET_BLOCK_SIZE:
        break;
    }

    ESP_LOGE(TAG,"Unsupported IOCTL 0x%02x", cmd);

    return -1;
}


static const ff_diskio_impl_t scsidev_impl = {
    .init = &ff_scsidev_initialize,
    .status = &ff_scsidev_status,
    .read = &ff_scsidev_read,
    .write = &ff_scsidev_write,
    .ioctl = &ff_scsidev_ioctl
};

#ifndef __linux__
static esp_err_t mount_to_vfs_fat(const esp_vfs_fat_mount_config_t *mount_config,
                           scsidev_t *card,
                           uint8_t pdrv,
                           const char *base_path)
{
    FATFS* fs = NULL;
    esp_err_t err;
    ff_diskio_register_scsidev(pdrv, card);

    ESP_LOGI(TAG, "using pdrv=%i", pdrv);
    char drv[3] = {(char)('0' + pdrv), ':', 0};

    // connect FATFS to VFS
    err = esp_vfs_fat_register(base_path, drv, mount_config->max_files, &fs);
    if (err == ESP_ERR_INVALID_STATE) {
        // it's okay, already registered with VFS
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_vfs_fat_register failed 0x(%x)", err);
        goto fail;
    }

    // Try to mount partition
    FRESULT res = f_mount(fs, drv, 1);

    if (res != FR_OK) {
        err = ESP_FAIL;
        ESP_LOGW(TAG, "failed to mount card (%d)", res);
        goto fail;
    }
    return ESP_OK;

fail:
    if (fs) {
        f_mount(NULL, drv, 0);
    }
    esp_vfs_fat_unregister_path(base_path);
    ff_diskio_unregister(pdrv);
    return err;
}

static void ff_diskio_register_scsidev(BYTE pdrv, scsidev_t* dev)
{
    scsi_devices[pdrv] = dev;
    ESP_LOGI(TAG, "Registering SCSI device 0x%02x", pdrv);
    ff_diskio_register(pdrv, &scsidev_impl);
}

static void ff_diskio_unregister_scsidev(BYTE pdrv, scsidev_t* dev)
{
    scsi_devices[pdrv] = dev;
    ff_diskio_unregister(pdrv);
}

esp_err_t vfs_fat_scsi_mount(const char* base_path,
                             scsidev_t *dev,
                             const esp_vfs_fat_mount_config_t* mount_config)
{
    esp_err_t err;
    // connect SCSI driver to FATFS

    BYTE pdrv = FF_DRV_NOT_USED;
    if (ff_diskio_get_drive(&pdrv) != ESP_OK || pdrv == FF_DRV_NOT_USED) {
        ESP_LOGI(TAG, "the maximum count of volumes is already mounted");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Using drive %02x", pdrv);

    err = mount_to_vfs_fat(mount_config, dev, pdrv, base_path);

    return err;
}

esp_err_t vfs_fat_scsi_umount(scsidev_t *dev, const char *mountpoint)
{
    BYTE pdrv = ff_diskio_get_pdrv_card(dev);

    if (pdrv == 0xff) {
        return ESP_ERR_INVALID_ARG;
    }

    char drv[3] = {(char)('0' + pdrv), ':', 0};
    f_mount(0, drv, 0);

    ff_diskio_unregister(pdrv);

    esp_err_t err = esp_vfs_fat_unregister_path(mountpoint);

    ff_diskio_unregister_scsidev(pdrv, dev);

    return err;
}
#else

esp_err_t vfs_fat_scsi_umount(scsidev_t *dev, const char *mountpoint)
{
    return 0;
}

#endif
