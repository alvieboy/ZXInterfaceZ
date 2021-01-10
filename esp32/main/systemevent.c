#include "systemevent.h"
#include "utils.h"
#include "log.h"

#define MAX_SYSTEMEVENT_HANDLERS 4

static struct {
    uint8_t mask;
    systemevent_handler_t handler;
    void *user;
} systemevent_handlers[MAX_SYSTEMEVENT_HANDLERS] = {};

void systemevent__register_handler(uint8_t mask, systemevent_handler_t handler, void *user)
{
    int i;
    for (i=0; i<ARRAY_SIZE(systemevent_handlers);i++) {
        if (systemevent_handlers[i].mask==0) {
            systemevent_handlers[i].mask = mask;
            systemevent_handlers[i].handler = handler;
            systemevent_handlers[i].user = user;
            return;
        }
    }
    ESP_LOGE("SYSTEMEVENT","Too many handlers!");
}

void systemevent__send_event(const systemevent_t *event)
{
    int i;
    for (i=0; i<ARRAY_SIZE(systemevent_handlers);i++) {
        if ((event->type & systemevent_handlers[i].mask)!=0) {
            if (systemevent_handlers[i].handler!=NULL)
                systemevent_handlers[i].handler(event, systemevent_handlers[i].user);
        }
    }
}
