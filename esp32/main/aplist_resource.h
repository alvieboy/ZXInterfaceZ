#ifndef __APLIST_RESOURCE_H__
#define __APLIST_RESOURCE_H__

#include "resource.h"
#include "json.h"

struct aplist_resource
{
    struct resource r;
    uint16_t buflen;
    uint16_t bufpos;
    uint16_t needlen;
    uint8_t *buf;
};

void aplist_resource__init(struct aplist_resource*);
void aplist_resource__clear(struct aplist_resource*);
void aplist_resource__resize(struct aplist_resource*, uint16_t len);
int aplist_resource__setnumaps(struct aplist_resource*, uint8_t num);
int aplist_resource__addap(struct aplist_resource*, uint8_t flags, const char *ssid, uint8_t ssidlen);

uint8_t aplist_resource__type(struct resource*);
uint16_t aplist_resource__len(struct resource*);
int aplist_resource__sendToFifo(struct resource *);

int aplist_resource__get_json(struct aplist_resource *,cJSON *root);

#define APLIST_RESOURCE_DEF  { \
    .type = &aplist_resource__type,  \
    .len = &aplist_resource__len,    \
    .sendToFifo  = &aplist_resource__sendToFifo, \
    .update = NULL \
    }

#endif
