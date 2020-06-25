#ifndef __ESPLOG_H__
#define __ESPLOG_H__

#include <inttypes.h>
#include <stdio.h>

#define do_log(m, tag,x...) do { \
    printf("%s %s ", m,tag); \
    printf(x);\
    printf("\n"); \
} while (0)

#define ESP_LOGI(tag,x...) do_log("I", tag, x)
#define ESP_LOGE(tag,x...) do_log("E", tag, x)
#define ESP_LOGW(tag,x...) do_log("W", tag, x)
#define ESP_LOGD(tag,x...) do_log("D", tag, x)

#endif
