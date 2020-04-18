#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>

void wifi__init(void);
bool wifi__isconnected(void);
bool wifi__issta(void);
bool wifi__scanning(void);

extern char wifi_ssid[33];

#endif

