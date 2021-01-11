#include <inttypes.h>

#if defined(__XTENSA__)

#ifndef bswap_16
#define bswap_16(a) ((((uint16_t) (a) << 8) & 0xff00) | (((uint16_t) (a) >> 8) & 0xff))
#endif

#ifndef bswap_32
#define bswap_32(a) ((((uint32_t) (a) << 24) & 0xff000000) | \
		     (((uint32_t) (a) << 8) & 0xff0000) | \
     		     (((uint32_t) (a) >> 8) & 0xff00) | \
     		     (((uint32_t) (a) >> 24) & 0xff))
#endif

#define __be16(x) bswap_16(x)
#define __be32(x) bswap_32(x)

#define __le16(x) (x)
#define __le32(x) (x)


#ifdef __CHECKER__
typedef uint16_t __attribute__((bitwise)) be_uint16_t;
typedef uint32_t __attribute__((bitwise)) be_uint32_t;
#else
typedef uint16_t be_uint16_t;
typedef uint32_t be_uint32_t;
#endif

typedef uint16_t le_uint16_t;
typedef uint32_t le_uint32_t;

#elif defined(__linux__)
#include <asm/byteorder.h>

typedef uint16_t be_uint16_t;
typedef uint32_t be_uint32_t;
typedef uint16_t le_uint16_t;
typedef uint32_t le_uint32_t;
#define __le16(x) (x)
#define __le32(x) (x)
#define __be16(x) __bswap_16(x)
#define __be32(x) __bswap_32(x)

#endif
