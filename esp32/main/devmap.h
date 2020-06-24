#ifndef __DEVMAP_H__
#define __DEVMAP_H__

#include "json.h"

void devmap__init(void);
void devmap__populate_devices(cJSON *node);

#define DEVMAP_MAX_DEVICES 4

#endif
