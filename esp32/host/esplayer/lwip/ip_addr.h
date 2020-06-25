#include <inttypes.h>
struct ip4_addr {
  uint32_t addr;
};

struct ip6_addr {
  uint32_t addr[4];
};

typedef struct ip4_addr ip4_addr_t;

typedef struct ip6_addr ip6_addr_t;
