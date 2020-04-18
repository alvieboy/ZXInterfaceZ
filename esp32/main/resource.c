#include "esp_log.h"
#include "defs.h"
#include "resource.h"
#include "flash_resource.h"
#include "fpga.h"

struct resource_entry
{
    uint8_t id;
    struct resource *res;
    struct resource_entry *next;
};

struct resource_entry *resource_root;

void resource__init(void)
{
    flash_resource__init();
    resource_root = NULL;
}

struct resource *resource__find(uint8_t id)
{
    struct resource_entry *node = resource_root;
    while (node) {
        if (node->id == id) {
            return node->res;
        }
    }
    // See if resource is in flash.

    struct flash_resource *r = flash_resource__find(id);
    if (r!=NULL) {
        return &r->r;
    }


    return NULL;
}

void resource__register(uint8_t id, struct resource *res)
{
    struct resource_entry **parent = &resource_root;
    while ((*parent)->next) {
        parent = &((*parent)->next);
    }
    struct resource_entry *this = (struct resource_entry*)malloc(sizeof(struct resource_entry));
    this->next = NULL;
    this->res = res;
    (*parent)->next = this;
}

int resource__sendtofifo(struct resource *res)
{
    if (res==NULL)
        return -1;

    if (res->update)
        res->update(res);

    // Reset resource fifo.
    fpga__set_trigger(FPGA_FLAG_TRIG_RESOURCEFIFO_RESET);

    // Send type first.
    uint8_t type = res->type(res);

    int r = fpga__load_resource_fifo(&type, sizeof(type), RESOURCE_DEFAULT_TIMEOUT);
    if (r<0)
        return r;

    uint16_t len = res->len(res);

    r = fpga__load_resource_fifo((uint8_t*)&len, sizeof(len), RESOURCE_DEFAULT_TIMEOUT);
    if (r<0)
        return r;

    return (res->sendToFifo(res));
}
