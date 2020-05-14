#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "defs.h"
#include "rom.h"
#include "fpga.h"

static const esp_partition_t *rom_partition = NULL;
static const uint8_t *rom_ptr = NULL;
static spi_flash_mmap_handle_t mmap_handle;

int rom__load_from_flash(void)
{
    esp_partition_iterator_t i =
        esp_partition_find(0x42, 0x00, NULL);
    if (i!=NULL) {
        rom_partition = esp_partition_get(i);

        esp_err_t err =esp_partition_mmap(rom_partition,
                                          0, /* Offset */
                                          rom_partition->size,
                                          SPI_FLASH_MMAP_DATA,
                                          (const void**)&rom_ptr,
                                          &mmap_handle);
        ESP_ERROR_CHECK(err);
        ESP_LOGI(TAG,"Mapped rom partition at %p", rom_ptr);
    } else {
        ESP_LOGW(TAG,"Cannot find rom partition!!!");

    }
    esp_partition_iterator_release(i);

    int r = fpga__upload_rom(rom_ptr, rom_partition->size);

    spi_flash_munmap(mmap_handle);
    return r;
}
