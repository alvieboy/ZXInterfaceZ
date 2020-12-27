#ifndef __ESPLOG_H__
#define __ESPLOG_H__

#include <inttypes.h>
#include <stdio.h>

void do_log(const char *type, const char *tag, const char *fmt, ...);

void set_logger(void (*)(const char *type, const char *tag, char *fmt, va_list ap));
/*
#define ESP_LOGI(tag,x...) do_log("I", tag, x)
#define ESP_LOGE(tag,x...) do_log("E", tag, x)
#define ESP_LOGW(tag,x...) do_log("W", tag, x)
#define ESP_LOGD(tag,x...) do_log("D", tag, x)
*/
#endif
