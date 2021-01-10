#ifndef __SYSTEMEVENT_H__
#define __SYSTEMEVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#define SYSTEMEVENT_TYPE_WIFI    (1<<0)
#define SYSTEMEVENT_TYPE_NETWORK (1<<1)
#define SYSTEMEVENT_TYPE_USB     (1<<2)
#define SYSTEMEVENT_TYPE_STORAGE (1<<3)

#define SYSTEMEVENT_WIFI_STATUS_CHANGED 0
#define SYSTEMEVENT_WIFI_SCAN_COMPLETED 1

#define SYSTEMEVENT_NETWORK_STATUS_CHANGED 0

#define SYSTEMEVENT_USB_DEVICE_CHANGED 0
#define SYSTEMEVENT_USB_OVERCURRENT 1

#define SYSTEMEVENT_STORAGE_BLOCKDEV_ATTACH 0
#define SYSTEMEVENT_STORAGE_BLOCKDEV_DETACH 1

typedef struct {
    uint8_t type;
    uint8_t event;
    void *ctxdata;
} systemevent_t;


void systemevent__send_event(const systemevent_t *event);
static inline void systemevent__send(uint8_t type, uint8_t event)
{
    systemevent_t ev;
    ev.type = type;
    ev.event = event;
    systemevent__send_event(&ev);
}

static inline void systemevent__send_with_ctx(uint8_t type, uint8_t event, void*ctx)
{
    systemevent_t ev;
    ev.type = type;
    ev.event = event;
    systemevent__send_event(&ev);
}

typedef void (*systemevent_handler_t)(const systemevent_t *event, void *user);


void systemevent__register_handler(uint8_t mask, systemevent_handler_t handler, void *user);


//extern void systemevent__handleevent(const systemevent_t *event);


#ifdef __cplusplus
}
#endif

#endif