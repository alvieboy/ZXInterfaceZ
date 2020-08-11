#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "defs.h"
#include "rom.h"
#include "fpga.h"
#include "memlayout.h"
#include <sys/stat.h>
#include "fileaccess.h"
#include "fcntl.h"

#ifndef __linux__

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

    //ASSERT(ROM_SIZE == NMI_ROM_SIZE);

    int r = fpga__upload_rom(NMI_ROM_BASEADDRESS, rom_ptr, ROM_SIZE);

    spi_flash_munmap(mmap_handle);
    return r;
}
#else
#include <string.h>
#include <unistd.h>

int rom__load_from_flash(void)
{
    uint8_t rom[16384];
    int fd;
    fd = open("intz.rom", O_RDONLY);
    if (!fd) {
        printf("Cannot open rom intz.rom: %s\n", strerror(errno));
        return -1;
    }
    read(fd, rom, sizeof(rom));
    close(fd);

    int r = fpga__upload_rom(MEMLAYOUT_ROM0_BASEADDRESS, rom, sizeof(rom));
    return r;
}

#endif


int rom__load_custom_from_file(const char *file)
{
    struct stat st;

    int fd = __open(file,O_RDONLY);
    if (fd<0)
        return -1;

    if (fstat(fd, &st)<0) {
        ESP_LOGE(TAG,"Cannot stat file");
        close(fd);
        return -1;
    }
    if (st.st_size > MEMLAYOUT_ROM2_SIZE) {
        ESP_LOGE(TAG,"File too large");
        close(fd);
        return -1;
    }
    int r = fpga__write_extram_block_from_file(MEMLAYOUT_ROM2_BASEADDRESS, fd, st.st_size, false);

    close(fd);

    return r;
}
