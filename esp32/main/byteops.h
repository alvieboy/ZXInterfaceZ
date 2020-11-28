#include <inttypes.h>

static inline uint32_t extractle16(const uint8_t *source)
{
    return (((uint16_t)source[1])<<8) + ((uint16_t)source[0]);
}

static inline uint32_t extractle24(const uint8_t *source)
{
    return (((uint32_t)source[2])<<16) +
        (((uint32_t)source[1])<<8) + ((uint32_t)source[0]);
}

static inline uint32_t extractbe32(const uint8_t *source)
{
    uint32_t ret = (((uint32_t)source[0]) << 24) +
        (((uint32_t)source[1]) << 16) +
        (((uint32_t)source[2]) << 8) +
        ((uint32_t)source[3]);
    return ret;
}

static inline uint32_t extractle32(const uint8_t *source)
{
    uint32_t ret = (((uint32_t)source[3]) << 24) +
        (((uint32_t)source[2]) << 16) +
        (((uint32_t)source[1]) << 8) +
        ((uint32_t)source[0]);
    return ret;
}

static inline uint16_t extractbe16(const uint8_t *source)
{
    uint16_t ret = ((((uint16_t)source[0]) << 8) +
        ((uint16_t)source[1]));
    return ret;
}


static inline uint8_t *putle16(uint8_t *dest, uint16_t val)
{
    *dest++ = val & 0xff;
    *dest++ = val >> 8;
    return dest;
}

static inline int putle16_c(uint8_t *dest, uint16_t val)
{
    *dest++ = val & 0xff;
    *dest++ = val >> 8;
    return 2;
}

static inline uint8_t *putle32(uint8_t *dest, uint32_t val)
{
    *dest++ = val & 0xff;
    *dest++ = val >> 8;
    *dest++ = val >> 16;
    *dest++ = val >> 24;
    return dest;
}
