#include "flash_pgm.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "minmax.h"
#include <string.h>

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

    handle->buffer = malloc(FLASH_BLOCK_SIZE);
    handle->bufferpos = 0;

    esp_partition_iterator_release(i);

    return r;
}

static int flash_pgm__program_buffer(flash_program_handle_t *handle)
{
    esp_err_t err = esp_partition_write(handle->partition,
                                        handle->offset,
                                        handle->buffer,
                                        handle->bufferpos);

    handle->offset += handle->bufferpos;
    handle->bufferpos = 0;

    return err;
}

int flash_pgm__program_chunk(flash_program_handle_t *handle, const uint8_t *data, unsigned len)
{
    while (len) {
        unsigned remain = FLASH_BLOCK_SIZE - handle->bufferpos;
        unsigned toload = MIN(remain, len);

        memcpy(&handle->buffer[handle->bufferpos], data, toload);

        handle->bufferpos += toload;

        if (handle->bufferpos==FLASH_BLOCK_SIZE) {
            if (flash_pgm__program_buffer(handle)<0) {
                return -1;
            }
        }
        len -= toload;
        data += toload;
    }
    return 0;
}

int flash_pgm__flush(flash_program_handle_t *handle)
{
    esp_err_t err = 0;

    if (handle->bufferpos > 0) {
        // Flush
        err = flash_pgm__program_buffer(handle);
    }
    if (handle->buffer) {
        free(handle->buffer);
        handle->buffer = NULL;
    }
    return err;
}

