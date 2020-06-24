#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>
#include "json.h"

void wifi__init(void);
bool wifi__isconnected(void);
bool wifi__issta(void);
bool wifi__scanning(void);
void wifi__get_conf_json(cJSON *node);

extern char wifi_ssid[33];

#endif

