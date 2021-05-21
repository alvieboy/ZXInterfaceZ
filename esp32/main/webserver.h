#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include "esp_http_server.h"

int webserver__init(void);
void webserver__decodeurl(char *src);
httpd_handle_t webserver__get_handle();

#endif
