#include "flash_pgm.h"
#include "esp_partition.h"
#include "esp_log.h"

#define TAG "FlashPGM"

int flash_pgm__prepare_programming(flash_program_handle_t *handle,
                                   esp_partition_type_t type,
                                   esp_partition_subtype_t subtype,
                                   const char *label,
                                   unsigned length)
{
    const esp_partition_t *fpga_partition = NULL;

    esp_partition_iterator_t i =
        esp_partition_find(type, subtype, label);

    if (i==NULL) {
        ESP_LOGE(TAG, "Cannot find partition");
        return -1;
    }

    fpga_partition = esp_partition_get(i);

    length = ALIGN(length, FLASH_BLOCK_SIZE);

    esp_err_t r = esp_partition_erase_range(fpga_partition, 0, length);
    if (r<0) {
        ESP_LOGE(TAG,"Cannot erase region");
        esp_partition_iterator_release(i);
        return -1;
    }

    handle->offset = 0;
    handle->partition = fpga_partition;

    esp_partition_iterator_release(i);

    return r;
}

int flash_pgm__program_chunk(flash_program_handle_t *handle, const uint8_t *data, unsigned len)
{
    esp_err_t err = esp_partition_write(handle->partition,
                                        handle->offset,
                                        data, len);
    if (err<0)
        return err;

    handle->offset += len;

    return 0;
}



