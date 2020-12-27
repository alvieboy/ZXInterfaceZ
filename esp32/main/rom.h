#ifndef __ROM_H__
#define __ROM_H__


#define ROM_SIZE 8192

typedef struct rom_model {
    uint32_t crc;
    const char *name;
} rom_model_t;

int rom__load_from_flash(void);
int rom__load_custom_from_file(const char *, unsigned address);
char *rom__get_version(void);

const rom_model_t *rom__get(uint8_t index);
const rom_model_t *rom__set_rom_crc(uint8_t index, uint32_t crc);

int rom__load_custom_routine(const uint8_t *data, unsigned size);

#endif
