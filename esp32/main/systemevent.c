#include "systemevent.h"
#include "utils.h"
#include "log.h"

#define MAX_SYSTEMEVENT_HANDLERS 4

static struct {
    uint8_t mask;
    systemevent_handler_t handler;
    void *user;
} systemevent_handlers[MAX_SYSTEMEVENT_HANDLERS] = {};

systemevent_handlerid_t systemevent__register_handler(uint8_t mask, systemevent_handler_t handler, void *user)
{
    unsigned i;
    for (i=0; i<ARRAY_SIZE(systemevent_handlers);i++) {
        if (systemevent_handlers[i].mask==0) {
            systemevent_handlers[i].mask = mask;
            systemevent_handlers[i].handler = handler;
            systemevent_handlers[i].user = user;
            return i;
        }
    }
    ESP_LOGE("SYSTEMEVENT","Too many handlers!");
    return -1;
}

int systemevent__unregister_handler(systemevent_handlerid_t handler)
{
    if (handler<0 || handler>=(int)ARRAY_SIZE(systemevent_handlers) ||
       systemevent_handlers[handler].mask==0)
        return -1;

    systemevent_handlers[handler].mask=0;
    return 0;
}

void systemevent__send_event(const systemevent_t *event)
{
    unsigned int i;
    for (i=0; i<ARRAY_SIZE(systemevent_handlers);i++) {
        if ((event->type & systemevent_handlers[i].mask)!=0) {
            if (systemevent_handlers[i].handler!=NULL)
                systemevent_handlers[i].handler(event, systemevent_handlers[i].user);
        }
    }
}
