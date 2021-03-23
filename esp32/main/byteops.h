#include <inttypes.h>
/**
 \ingroup misc
 \defgroup byteops Byte operation routines
 \brief Byte operation routines
 */

/**
 \ingroup byteops
 \brief Extract a 16-bit unsigned Little Endian from a byte array
 \param source The byte array
 \return The corresponding 16-bit value
 */
static inline uint16_t extractle16(const uint8_t *source)
{
    return (((uint16_t)source[1])<<8) + ((uint16_t)source[0]);
}

/**
 \ingroup byteops
 \brief Extract a 24-bit unsigned Little Endian from a byte array
 \param source The byte array
 \return The corresponding 24-bit value
 */
static inline uint32_t extractle24(const uint8_t *source)
{
    return (((uint32_t)source[2])<<16) +
        (((uint32_t)source[1])<<8) + ((uint32_t)source[0]);
}


/**
 \ingroup byteops
 \brief Extract a 32-bit unsigned Big Endian from a byte array
 \param source The byte array
 \return The corresponding 32-bit value
 */

static inline uint32_t extractbe32(const uint8_t *source)
{
    uint32_t ret = (((uint32_t)source[0]) << 24) +
        (((uint32_t)source[1]) << 16) +
        (((uint32_t)source[2]) << 8) +
        ((uint32_t)source[3]);
    return ret;
}

/*
 \ingroup byteops
 \brief Extract a 32-bit unsigned Little Endian from a byte array
 \param source The byte array
 \return The corresponding 32-bit value
 */
static inline uint32_t extractle32(const uint8_t *source)
{
    uint32_t ret = (((uint32_t)source[3]) << 24) +
        (((uint32_t)source[2]) << 16) +
        (((uint32_t)source[1]) << 8) +
        ((uint32_t)source[0]);
    return ret;
}

/**
 \ingroup byteops
 \brief Extract a 16-bit unsigned Big Endian from a byte array
 \param source The byte array
 \return The corresponding 16-bit value
 */
static inline uint16_t extractbe16(const uint8_t *source)
{
    uint16_t ret = ((((uint16_t)source[0]) << 8) +
        ((uint16_t)source[1]));
    return ret;
}


/**
 \ingroup byteops
 \brief Put a 16-bit unsigned in a byte array, Little Endian format
 \param dest The byte array
 \param val The value to be placed in the byte array
 \return A pointer to the location past the value that was stored
 */
static inline uint8_t *putle16(uint8_t *dest, uint16_t val)
{
    *dest++ = val & 0xff;
    *dest++ = val >> 8;
    return dest;
}

/**
 \ingroup byteops
 \brief Put a 16-bit unsigned in a byte array, Little Endian format
 \param dest The byte array
 \param val The value to be placed in the byte array
 \return The number of bytes written
 */
static inline int putle16_c(uint8_t *dest, uint16_t val)
{
    *dest++ = val & 0xff;
    *dest++ = val >> 8;
    return 2;
}

/**
 \ingroup byteops
 \brief Put a 32-bit unsigned in a byte array, Little Endian format
 \param dest The byte array
 \param val The value to be placed in the byte array
 \return A pointer to the location past the value that was stored
 */
static inline uint8_t *putle32(uint8_t *dest, uint32_t val)
{
    *dest++ = val & 0xff;
    *dest++ = val >> 8;
    *dest++ = val >> 16;
    *dest++ = val >> 24;
    return dest;
}
