#include <inttypes.h>
struct ip4_addr {
  uint32_t addr;
};

struct ip6_addr {
  uint32_t addr[4];
};

typedef struct ip4_addr ip4_addr_t;

typedef struct ip6_addr ip6_addr_t;

#ifndef MACSTR
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif
