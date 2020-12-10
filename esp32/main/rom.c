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

static char romversion[64];

static int rom__read_version(const char *file);

int rom__load_from_flash(void)
{
    /*int size;
    void *data = readfile(ROM_FILENAME, &size);

    if (data==NULL) {
        ESP_LOGE(TAG,"Cannot load ROM file");
    }

    int r = fpga__upload_rom(NMI_ROM_BASEADDRESS, data, size);

    free(data);

    return r;*/
    romversion[0] = '\0';
    if (rom__read_version(ROM_FILENAME)<0)
        return -1;

    return rom__load_custom_from_file(ROM_FILENAME, NMI_ROM_BASEADDRESS);
}

char *rom__get_version()
{
    return romversion;
}

static int rom__read_version(const char *file)
{
    struct stat st;
    int fd = __open(file,O_RDONLY);

    if (fd<0)
        return -1;

    if (fstat(fd, &st)<0) {
        return -1;
    }

    if (st.st_size<0x1F00) {
        close(fd);
        return -1; // Too short
    }

    if (lseek(fd, 0x1EC0, SEEK_SET)!=0x1EC0)
    {
        close(fd);
        return -1;
    }
    int r = read(fd, romversion, sizeof(romversion));

    close(fd);

    if (r<0)
        return r;

    // Forcibly NULL-terminate string
    romversion[63] = '\0';

    // Sanity check.
    int i;
    for (i=0;i< sizeof(romversion) && romversion[i]; i++) {
        if (romversion[i]<' ' || romversion[i]>127) {
            return -1;
        }
    }

    return 0;
}

int rom__load_custom_from_file(const char *file, unsigned address)
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

    int r = fpga__write_extram_block_from_file(address, fd, st.st_size, false);

    close(fd);

    return r;
}
