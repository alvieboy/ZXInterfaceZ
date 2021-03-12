#include "scsidev.h"
#include <string.h>
#include "log.h"
#include "byteorder.h"
#include "systemevent.h"
#include <stdio.h>
#include "bitmap_allocator.h"
#include "esp_assert.h"

#define SCSIDEVTAG "SCSIDEV"

static bool scsidev_initialised = false;

static bitmap32_t scsi_id_bitmap;

static int scsidev__ensure_ready(scsidev_t *dev);
static int scsidev__read_capacity(scsidev_t *dev, uint8_t *status, struct scsi_sbc_read_capacity *data);

static void scsidev__initialize()
{
    bitmap_allocator__init(&scsi_id_bitmap);
}

static void scsidev__register(scsidev_t *dev)
{
    if (!scsidev_initialised)
        scsidev__initialize();

    int newid = bitmap_allocator__alloc(&scsi_id_bitmap);

    assert(newid>=0);

    dev->id = newid;
}

int scsidev__unregister(scsidev_t *dev)
{
    bitmap_allocator__release(&scsi_id_bitmap, dev->id);
    return 0;
}


void scsidev__getname(scsidev_t *dev, char *out)
{
    sprintf(out,"%sblk%d", dev->backend, dev->id);
}

static void scsidev__dump_inquiry_info(struct scsi_sbc_inquiry_data *data)
{
    char vendor[sizeof(data->vendor_id)+1];
    char product[sizeof(data->product_id)+1];
    char revision[sizeof(data->product_revision)+1];


    memcpy(vendor, data->vendor_id, sizeof(data->vendor_id));
    vendor[ sizeof(data->vendor_id) -1 ] = '\0';

    memcpy(product, data->product_id, sizeof(data->product_id));
    product[ sizeof(data->product_id) -1 ] = '\0';

    memcpy(revision, data->product_revision, sizeof(data->product_revision));
    revision[ sizeof(data->product_revision) -1 ] = '\0';

    ESP_LOGI(SCSIDEVTAG,"SCSI block device: %s %s %s",
             vendor, product, revision);

}

static int scsidev__read_size(scsidev_t *dev)
{
    struct scsi_sbc_read_capacity cap;
    uint8_t status;
    int r = scsidev__read_capacity(dev, &status, &cap);
    if (r<0) {
        ESP_LOGE(SCSIDEVTAG,"Error requesting capacity");
        return r;
    }
    if (status!=0x00) {
        // TBD: recover.
        if (scsidev__ensure_ready(dev)<0)
            return -1;
        r = scsidev__read_capacity(dev, &status, &cap);
        if (r<0) {
            ESP_LOGE(SCSIDEVTAG,"Error requesting capacity");
            return r;
        }
    }
    if (status!=0x00) {
        ESP_LOGE(SCSIDEVTAG,"Error status %02x requesting capacity", status);
        return -1;
    }

    dev->sector_count = __be32(cap.lba);
    dev->sector_size = (uint16_t)__be32(cap.blocksize);

    ESP_LOGI(SCSIDEVTAG,"Device: %u sectors of %u bytes", dev->sector_count, dev->sector_size);
    return 0;
}

int scsidev__init(scsidev_t *dev, const struct scsiblockfn *fn, void *pvt)
{
    struct scsi_sbc_inquiry_data inquiry;
    uint8_t status;
    dev->fn = fn;
    dev->pvt = pvt;


    ESP_LOGI(SCSIDEVTAG, "Sending INQUIRY");
    if (scsidev__inquiry(dev, &status, &inquiry)<0) {
        ESP_LOGE(SCSIDEVTAG, "Sending INQUIRY failed!");
        return -1;
    }

    scsidev__dump_inquiry_info(&inquiry);

    if (scsidev__ensure_ready(dev)<0) {
        ESP_LOGE(SCSIDEVTAG, "Unit NOT ready!");
        return -1;
    }

    if (scsidev__read_size(dev)<0) {
        ESP_LOGE(SCSIDEVTAG, "Cannot read capacity!");
        return -1;
    }

    scsidev__register(dev);

    systemevent__send_with_ctx(SYSTEMEVENT_TYPE_BLOCKDEV,
                               SYSTEMEVENT_BLOCKDEV_ATTACH, dev);

    return 0;
}

int scsidev__deinit(scsidev_t *dev)
{
    scsidev__unregister(dev);
    systemevent__send_with_ctx(SYSTEMEVENT_TYPE_BLOCKDEV,
                               SYSTEMEVENT_BLOCKDEV_DETACH, dev);
    return 0;
}

static int scsidev__request_sense(scsidev_t *dev, uint8_t *status, struct scsi_sbc_sense_data *data)
{
    struct scsi_cdb_request_sense *cdb_request_sense = (struct scsi_cdb_request_sense *)dev->cdb;

    memset(dev->cdb, 0, sizeof(dev->cdb));

    cdb_request_sense->opcode = SBC_CMD_REQUEST_SENSE;
    cdb_request_sense->length = sizeof(struct scsi_sbc_sense_data);

    return dev->fn->read(dev->pvt,
                         dev->cdb,
                         sizeof(struct scsi_cdb_request_sense),
                         (uint8_t*)data,
                         sizeof(struct scsi_sbc_sense_data),
                         status
                        );
}



int scsidev__inquiry(scsidev_t *dev, uint8_t *status, struct scsi_sbc_inquiry_data *data)
{
    struct scsi_cdb_inquiry *cdb_inquiry = (struct scsi_cdb_inquiry*)dev->cdb;

    memset(dev->cdb, 0, sizeof(dev->cdb));

    cdb_inquiry->opcode = SBC_CMD_INQUIRY;
    cdb_inquiry->length = __be16(sizeof(struct scsi_sbc_inquiry_data));

    return dev->fn->read(dev->pvt,
                         dev->cdb,
                         sizeof(struct scsi_cdb_inquiry),
                         (uint8_t*)data,
                         sizeof(struct scsi_sbc_inquiry_data),
                         status
                        );
}

static int scsidev__read_capacity(scsidev_t *dev, uint8_t *status, struct scsi_sbc_read_capacity *data)
{
    struct scsi_cdb_read_capacity_10 *cdb_inquiry = (struct scsi_cdb_read_capacity_10*)dev->cdb;

    memset(dev->cdb, 0, sizeof(dev->cdb));

    cdb_inquiry->opcode = SBC_CMD_READ_CAPACITY;

    return dev->fn->read(dev->pvt,
                         dev->cdb,
                         sizeof(struct scsi_cdb_read_capacity_10),
                         (uint8_t*)data,
                         sizeof(struct scsi_sbc_read_capacity),
                         status
                        );
}

static int scsidev__test_unit_ready(scsidev_t *dev, uint8_t *status)
{
    struct scsi_cdb_test_unit_ready * cdb_test_unit_ready = (struct scsi_cdb_test_unit_ready*)dev->cdb;

    memset(dev->cdb, 0, sizeof(dev->cdb));

    cdb_test_unit_ready->opcode = SBC_CMD_TEST_UNIT_READY;

    BUFFER_LOGI(SCSIDEVTAG,"TestUnitReady", dev->cdb, sizeof(dev->cdb));

    int r = dev->fn->write(dev->pvt,
                           dev->cdb,
                           sizeof(struct scsi_cdb_test_unit_ready),
                           NULL,
                           0,
                           status
                          );
    if (r<0)
        return -1;


    return 0;
}

static int scsidev__read_sense_data(scsidev_t *dev)
{
    int r;
    uint8_t status;
    // Check condition.
    struct scsi_sbc_sense_data sensedata;

    ESP_LOGI(SCSIDEVTAG, "Request sense into %p", &sensedata);

    r = scsidev__request_sense(dev, &status, &sensedata);

    if (r<0)
        return -1;

    ESP_LOGI(SCSIDEVTAG, "Sense data: status %02x, key %02x code %02x code_qual %02x",
             status,
             sensedata.sense_key,
             sensedata.additional_sense_code,
             sensedata.additional_sense_code_qual);

    return r;
}

static int scsidev__phase_error(scsidev_t *dev)
{
    ESP_LOGE(SCSIDEVTAG, "Phase error!!!!");
    return -1;
}

static int scsidev__ensure_ready(scsidev_t *dev)
{
    uint8_t status;
    int r;

    r = scsidev__test_unit_ready(dev, &status);
    if (r<0) {
        return -1;
    }

    switch (status) {
    case 0:
        return 0;
    case 1: // Command failed
        r = scsidev__read_sense_data(dev);
        break;
    case 2:
        r = scsidev__phase_error(dev);
        break;
    }

    if (r<0)
        return -1;

    if (scsidev__test_unit_ready(dev, &status)) {
        return -1;
    }

    if (status!=0) {
        ESP_LOGE(SCSIDEVTAG, "Cannot clear device status!");
        return -1;
    }

    return 0;
}

int scsidev__read(scsidev_t *dev, uint8_t *target, sector_t sector, unsigned sector_count, uint8_t *status)
{
    struct scsi_cdb_read_10 *cdb_read_10 = (struct scsi_cdb_read_10*)dev->cdb;

    memset(dev->cdb, 0, sizeof(dev->cdb));

    cdb_read_10->opcode = SBC_CMD_READ_10;
    cdb_read_10->length = __be16(sector_count);
    cdb_read_10->lba = __be32(sector);

    return dev->fn->read(dev->pvt,
                         dev->cdb,
                         sizeof(struct scsi_cdb_read_10),
                         target,
                         sector_count * dev->sector_size,
                         status
                        );

}

int scsidev__write(scsidev_t *dev, const uint8_t *source, sector_t sector, unsigned sector_count, uint8_t *status)
{
    struct scsi_cdb_write_10 *cdb_write_10 = (struct scsi_cdb_write_10*)dev->cdb;

    memset(dev->cdb, 0, sizeof(dev->cdb));

    cdb_write_10->opcode = SBC_CMD_WRITE_10;
    cdb_write_10->length = __be16(sector_count);
    cdb_write_10->lba = __be32(sector);

    return dev->fn->write(dev->pvt,
                          dev->cdb,
                          sizeof(struct scsi_cdb_read_10),
                          source,
                          sector_count * dev->sector_size,
                          status
                         );


    return -1;
}

