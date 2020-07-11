#include "esp_partition.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

// Hardcoded partition

#define BINFILE "partition.bin"

#if 0
# ESP-IDF Partition Table
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  0,    0,       0x10000, 1M,
ota_0,    0,    ota_0,  0x110000, 1M,
ota_1,    0,    ota_1,  0x210000, 1M,
fpga,     0x40, 0x00,   0x310000, 0x5A000,
mainrom,  0x42, 0x00,   0x36A000, 0x4000,
config,   data, spiffs, 0x36E000, 0x8000
resources,data, spiffs, 0x376000, 0x8A000

typedef struct {
    esp_flash_t* flash_chip;            /*!< SPI flash chip on which the partition resides */
    esp_partition_type_t type;          /*!< partition type (app/data) */
    esp_partition_subtype_t subtype;    /*!< partition subtype */
    uint32_t address;                   /*!< starting address of the partition in flash */
    uint32_t size;                      /*!< size of the partition, in bytes */
    char label[17];                     /*!< partition label, zero-terminated ASCII string */
    bool encrypted;                     /*!< flag is set to true if partition is encrypted */
} esp_partition_t;
#endif

typedef struct esp_partition_iterator_opaque_ {
    const esp_partition_t* info;                // pointer to info (it is redundant, but makes code more readable)
} esp_partition_iterator_opaque_t;



static const esp_partition_t partitions[] = {
    { NULL, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS,       0x9000, 0x4000, "nvs", false },
    { NULL, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA,       0xd000, 0x2000, "ota", false },
    { NULL, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY,       0xf000, 0x1000, "phy_init", false },
    { NULL, ESP_PARTITION_TYPE_APP,  ESP_PARTITION_SUBTYPE_APP_FACTORY,   0x10000,0x100000, "factory", false },
    { NULL, ESP_PARTITION_TYPE_APP,  ESP_PARTITION_SUBTYPE_APP_OTA_0,    0x110000,0x100000, "ota_0", false },
    { NULL, ESP_PARTITION_TYPE_APP,  ESP_PARTITION_SUBTYPE_APP_OTA_0,    0x210000,0x100000, "ota_1", false },
    { NULL, 0x40,                    0x00,                               0x310000,0x5A000, "fpga", false },
    { NULL, 0x42,                    0x00,                               0x36A000,0x4000, "mainrom", false },
    { NULL, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS,  0x36E000,0x8000, "config", false },
    { NULL, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS,  0x376000,0x8A000, "resources", false },
};

#define FLASH_CHIP_SIZE (4*1024*1024*8)

static int flashfd = -1;

int partition_init()
{
    flashfd = open(BINFILE, O_RDWR|O_CREAT, 0666);
    if (flashfd<0) {
        flashfd = open(BINFILE, O_RDWR|O_CREAT|O_TRUNC, 0666);
    }
    if (flashfd<0) {
        fprintf(stderr, "Cannot open %s for read/write: %s\n", BINFILE, strerror(errno));
        return -1;
    }
    ftruncate(flashfd, FLASH_CHIP_SIZE);
    return 0;
}
/*P
 * @return ESP_OK, if data was written successfully;
 *         ESP_ERR_INVALID_ARG, if dst_offset exceeds partition size;
 *         ESP_ERR_INVALID_SIZE, if write would go out of bounds of the partition;
 *         or one of error codes from lower-level flash driver.
 */
esp_err_t esp_partition_write(const esp_partition_t* partition,
                              size_t dst_offset, const void* src, size_t size)
{
    if ((dst_offset+size-1) > partition->size) {
        return ESP_ERR_INVALID_SIZE;
    }
    lseek( flashfd, partition->address + dst_offset, SEEK_SET);
    if (write( flashfd, src, size)==size)
        return 0;

    return -1;
}


/*
 * @return ESP_OK, if the range was erased successfully;
 *         ESP_ERR_INVALID_ARG, if iterator or dst are NULL;
 *         ESP_ERR_INVALID_SIZE, if erase would go out of bounds of the partition;
 *         or one of error codes from lower-level flash driver.
 */
esp_err_t esp_partition_erase_range(const esp_partition_t* partition,
                                    size_t offset, size_t size)
{
    uint8_t eraseblock[4096];

    if ((size & 4095)!=0)
        return ESP_ERR_INVALID_SIZE;

    if ((offset+size-1) > partition->size) {
        return ESP_ERR_INVALID_SIZE;
    }

    lseek( flashfd, partition->address + offset, SEEK_SET);
    memset(eraseblock, 0xff, sizeof(eraseblock));

    while (size) {
        if (write( flashfd, eraseblock, sizeof(eraseblock))!=sizeof(eraseblock)) {
            perror("write");
            return -1;
        }
        size-=sizeof(eraseblock);
    }

    return 0;
}



esp_partition_iterator_t esp_partition_find(esp_partition_type_t type, esp_partition_subtype_t subtype, const char* label)
{
    unsigned i;
    for (i=0;i<sizeof(partitions)/sizeof(partitions[0]);i++) {
        if ((partitions[i].type == type) &&
            ( partitions[i].subtype == subtype)) {
            if (label) {
                if (strcmp(label, partitions[i].label)!=0) {
                    continue;
                }
            }
            esp_partition_iterator_opaque_t *r = malloc(sizeof(esp_partition_iterator_opaque_t));
            r->info = &partitions[i];
            return r;
        }
    }
    return NULL;
}

const esp_partition_t* esp_partition_get(esp_partition_iterator_t iterator)
{
    return iterator->info;
}

void esp_partition_iterator_release(esp_partition_iterator_t iterator)
{
    if (iterator)
        free(iterator);
}

