#include "mdns.h"

int mdns_init()
{
    return 0;
}

int mdns_instance_name_set(const char *c)
{
    return 0;
}

int mdns_hostname_set(const char *c)
{
    return 0;
}

int mdns_service_add(const char * instance_name, const char * service_type,
                     const char * proto, uint16_t port, mdns_txt_item_t txt[], size_t num_items)
{
    return 0;
}

int mdns_service_instance_name_set(const char * service_type, const char * proto, const char * instance_name)
{
    return 0;
}

void mdns_free()
{
}
