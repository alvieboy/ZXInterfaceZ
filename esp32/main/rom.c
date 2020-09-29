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

#define ROM_FILENAME "/spiffs/intz.rom"

int rom__load_from_flash(void)
{
    int size;
    void *data = readfile(ROM_FILENAME, &size);

    if (data==NULL) {
        ESP_LOGE(TAG,"Cannot load ROM file");
    }

    int r = fpga__upload_rom(NMI_ROM_BASEADDRESS, data, size);

    free(data);

    return r;
}

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

    ESP_LOGI(TAG, "ROM: loading %ld bytes", st.st_size);

    int r = fpga__write_extram_block_from_file(MEMLAYOUT_ROM2_BASEADDRESS, fd, st.st_size, false);

    close(fd);

    return r;
}
