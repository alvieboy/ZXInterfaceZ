#ifndef __SYSTEMEVENT_H__
#define __SYSTEMEVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#define SYSTEMEVENT_TYPE_WIFI    (1<<0)
#define SYSTEMEVENT_TYPE_NETWORK (1<<1)

#define SYSTEMEVENT_WIFI_STATUS_CHANGED 0
#define SYSTEMEVENT_WIFI_SCAN_COMPLETED 1

#define SYSTEMEVENT_NETWORK_STATUS_CHANGED 0



typedef struct {
    uint8_t type;
    uint8_t event;
} systemevent_t;


void systemevent__send(const systemevent_t *event);

extern void systemevent__handleevent(const systemevent_t *event);


#ifdef __cplusplus
}
#endif

#endif