#include "systemevent.h"
#include "log.h"
#include "scsidev.h"
#include "scsi_diskio.h"

#define STORAGETAG "STORAGE"

void storage__attach_scsidev(scsidev_t *dev)
{
    char name[16];
    scsidev__getname(dev, name);

}

static void storage__handleevent(const systemevent_t *event, void*user)
{
    ESP_LOGI(STORAGETAG, "Storage event 0x%02x", event->event);

    switch (event->event) {
    case SYSTEMEVENT_STORAGE_BLOCKDEV_ATTACH:
        storage__attach_scsidev(event->ctxdata);
        break;
    default:
        break;
    }
}

void storage__init()
{
    systemevent__register_handler(SYSTEMEVENT_TYPE_STORAGE, storage__handleevent, NULL);
}
