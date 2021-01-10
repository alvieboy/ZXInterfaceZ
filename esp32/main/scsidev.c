#include "scsidev.h"
#include <string.h>
#include "log.h"
#include "byteorder.h"
#include "systemevent.h"
#include <stdio.h>

#define SCSIDEVTAG "SCSIDEV"

static uint8_t scsi_id = 0;

static int scsidev__ensure_ready(scsidev_t *dev);

static void scsidev__register(scsidev_t *dev)
{
    dev->id = scsi_id;
    scsi_id++;
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


int scsidev__init(scsidev_t *dev, const struct scsiblockfn *fn, void *pvt)
{
    struct scsi_sbc_inquiry_data inquiry;
    uint8_t status;
    dev->fn = fn;
    dev->pvt = pvt;


    if (scsidev__ensure_ready(dev)<0) {
        ESP_LOGE(SCSIDEVTAG, "Unit NOT ready!");
        return -1;
    }

    ESP_LOGI(SCSIDEVTAG, "Sending INQUIRY");
    if (scsidev__inquiry(dev, &status, &inquiry)<0) {
        ESP_LOGE(SCSIDEVTAG, "Sending INQUIRY failed!");
        return -1;
    }

    scsidev__dump_inquiry_info(&inquiry);

    scsidev__register(dev);

    systemevent__send_with_ctx(SYSTEMEVENT_TYPE_STORAGE, SYSTEMEVENT_STORAGE_BLOCKDEV_ATTACH, dev);

    return 0;
}

static int scsidev__request_sense(scsidev_t *dev, uint8_t *status, struct scsi_sbc_sense_data *data)
{
    memset(dev->cdb, 0, sizeof(dev->cdb_request_sense));

    dev->cdb_request_sense.opcode = SBC_CMD_REQUEST_SENSE;
    dev->cdb_request_sense.length = sizeof(struct scsi_sbc_sense_data);

    return dev->fn->read(dev->pvt,
                         dev->cdb,
                         sizeof(dev->cdb_inquiry),
                         (uint8_t*)data,
                         sizeof(struct scsi_sbc_sense_data),
                         status
                        );
}



int scsidev__inquiry(scsidev_t *dev, uint8_t *status, struct scsi_sbc_inquiry_data *data)
{
    memset(dev->cdb, 0, sizeof(dev->cdb_inquiry));

    dev->cdb_inquiry.opcode = SBC_CMD_INQUIRY;
    dev->cdb_inquiry.length = __be16(sizeof(struct scsi_sbc_inquiry_data));

    return dev->fn->read(dev->pvt,
                         dev->cdb,
                         sizeof(dev->cdb_inquiry),
                         (uint8_t*)data,
                         sizeof(struct scsi_sbc_inquiry_data),
                         status
                        );
}

static int scsidev__test_unit_ready(scsidev_t *dev, uint8_t *status)
{
    memset(dev->cdb, 0, sizeof(dev->cdb_test_unit_ready));

    dev->cdb_inquiry.opcode = SBC_CMD_TEST_UNIT_READY;

    int r = dev->fn->read(dev->pvt,
                          dev->cdb,
                          sizeof(dev->cdb_test_unit_ready),
                          NULL,
                          0,
                          status
                         );
    if (r<0)
        return -1;


    return 0;
}

static int scsidev__ensure_ready(scsidev_t *dev)
{
    uint8_t status;
    int r;

    if (scsidev__test_unit_ready(dev, &status)) {
        return -1;
    }

    if (status!=0) {
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

        if (scsidev__test_unit_ready(dev, &status)) {
            return -1;
        }
        if (status!=0) {
            ESP_LOGE(SCSIDEVTAG, "Cannot clear device status!");
            return -1;
        }
    }
    return 0;

}

int scsidev__read(scsidev_t *dev, uint8_t *target, sector_t sector, unsigned size, uint8_t *status)
{
    memset(dev->cdb, 0, sizeof(dev->cdb_read_10));

    dev->cdb_read_10.opcode = SBC_CMD_READ_10;
    dev->cdb_read_10.length = __be16(size);
    dev->cdb_read_10.lba = __be32(sector);

    return dev->fn->read(dev->pvt,
                         dev->cdb,
                         sizeof(dev->cdb_read_10),
                         target,
                         size,
                         status
                        );

}

int scsidev__write(scsidev_t *dev, const uint8_t *source, sector_t sector, unsigned size, uint8_t *status)
{
    return -1;
}

