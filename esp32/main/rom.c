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
#include "flash_resource.h"

#define TAG "ROM"
static char romversion[64];

const rom_model_t *detected_rom[4] ={ NULL };

/*
 * NOTE NOTE NOTE
 *
 * This is *NOT* a full CRC32 of the ROM image, instead we use data each 64 bytes to compute
 * a CRC32 (total 256 bytes per 16KB of ROM). This makes calculating the CRC much faster, and
 * it is enough to detect each ROM. This also means you cannot use the pre-computed CRC which
 * are available on the Internet.
 *
*/
static const rom_model_t rom_models[] = {
    { 0xf20fe685, "BASIC for 16/48K models" },
    { 0x337e8d68, "Spanish 48K ROM" },
    { 0x97c8b8ab, "Didaktik" },
    { 0x4a15f949, "English 128 ROM 0 (128K editor and menu)"},
    { 0x4942ac90, "English 128 ROM 1 (48K BASIC)" },
    { 0xa3ac08d4, "Spanish 128 ROM 0 (128K editor and menu)" },
    { 0x4eb16388, "Spanish +2 ROM 0 (128K editor & menu)" },
    { 0x29eaf9c8, "Spanish 128 ROM 1 (48K BASIC)" },
    { 0x0efe0f61, "Spanish +2 ROM 1 (48K BASIC)" },
    { 0x99e01bf5, "+2A (rom 0)" },
    { 0x538ed9fd, "+2A (rom 1)" },
    { 0x1876d007, "+3 (rom 0)" },
    { 0xc0bd92b1, "+3 (rom 1)" },
    { 0xbc11e448, "+3 (rom 2)" },
    { 0x8623592e, "+3 (rom 3)" }
};


void rom__get_rom_filename(char *target)
{
    sprintf(target, "%s/intz.rom", flash_resource__get_root());
}

static int rom__read_version(const char *file);


const rom_model_t *rom__get(uint8_t index)
{
    index &=3;
    return detected_rom[index];
}

const rom_model_t *rom__set_rom_crc(uint8_t index, uint32_t crc)
{
    unsigned i;
    index &=3;
    detected_rom[index] = NULL;

    for(i=0;i<sizeof(rom_models)/sizeof(rom_models[0]);i++) {
        if (rom_models[i].crc==crc) {
            detected_rom[index] = &rom_models[i];
            break;
        }
    }
    return detected_rom[index];
}

int rom__load_from_flash(void)
{
    char romfilename[128];

    rom__get_rom_filename(romfilename);
    romversion[0] = '\0';
    if (rom__read_version(romfilename)<0)
        return -1;

    return rom__load_custom_from_file(romfilename, NMI_ROM_BASEADDRESS);
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

    int r = fpga__write_extram_block_from_file(address, fd, st.st_size);

    close(fd);

    return r;
}

int rom__load_custom_routine(const uint8_t *data, unsigned size)
{
    union u32 freearea;
    // Read out free area
    fpga__read_extram_block(NMI_ROM_BASEADDRESS + 0x0006, &freearea.w32, 2);

    uint16_t start = (uint16_t)freearea.w8[0] + (((uint16_t)freearea.w8[1])<<8);

    ESP_LOGI(TAG,"Start of ROM custom area: %02x\n", start);

    fpga__write_extram_block(NMI_ROM_BASEADDRESS+start, data, size);
    return 0;
}
