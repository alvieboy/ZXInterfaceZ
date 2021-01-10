#ifndef __SCSI_DISKIO_H__
#define __SCSI_DISKIO_H__

#include "scsidev.h"
#include "esp_vfs_fat.h"

esp_err_t vfs_fat_scsi_mount(const char* base_path,
                             scsidev_t *dev,
                             const esp_vfs_fat_mount_config_t* mount_config);


#endif