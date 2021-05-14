#ifndef __FIRMWARE_WS_H__
#define __FIRMWARE_WS_H__

#include "esp_http_server.h"
#include "stream.h"

esp_err_t firmware_ws__firmware_upgrade(httpd_req_t *req);
int firmware_ws__read(httpd_req_t *req, void *buf, size_t size, bool block);

#endif
