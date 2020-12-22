#ifndef __ESP_ROM_CRC__
#define __ESP_ROM_CRC__

#define CRCPOLY_LE 0xedb88320

static inline uint32_t crc32_le(uint32_t crc, unsigned char const *p, size_t len)
{
    int i;
    while (len--) {
        crc ^= *p++;
        for (i = 0; i < 8; i++)
            crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
    }
    return crc;
}

#endif
