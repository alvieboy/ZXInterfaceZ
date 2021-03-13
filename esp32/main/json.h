#include <cJSON.h>
#include "lwip/ip.h"


cJSON *json__load_from_file(const char *filename);
char *json__get_string_alloc(cJSON*, const char*);
const char *json__get_string(cJSON*, const char*);
int json__get_ip(cJSON*node, const char *name, ip4_addr_t *addr);
int json__get_integer_default(cJSON*, const char*, int def);
int json__get_integer(cJSON*, const char*);
const char *json__get_key(cJSON*node);
