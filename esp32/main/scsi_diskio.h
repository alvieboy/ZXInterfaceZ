#ifndef __SCSI_DISKIO_H__
#define __SCSI_DISKIO_H__

#include "scsidev.h"
#include "esp_vfs_fat.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t vfs_fat_scsi_mount(const char* base_path,
                             scsidev_t *dev,
                             const esp_vfs_fat_mount_config_t* mount_config);

esp_err_t vfs_fat_scsi_umount(scsidev_t *dev, const char *mountpoint);

#ifdef __cplusplus
}
#endif

#endif
