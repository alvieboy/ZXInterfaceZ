#ifndef __SCSIDEV_H__
#define __SCSIDEV_H__

#include "scsi.h"

typedef uint32_t sector_t;

struct scsiblockfn {
    int (*write)(void *, uint8_t *cdb, unsigned cdb_size, const uint8_t *data, unsigned datalen, uint8_t *status_out);
    int (*read)(void *, uint8_t *cdb, unsigned cdb_size, uint8_t *data, unsigned datalen, uint8_t *status_out);
};

typedef struct
{
    //uint8_t lun;
    union {
        uint8_t cdb[16];
        struct scsi_cdb_inquiry cdb_inquiry;
        struct scsi_cdb_test_unit_ready cdb_test_unit_ready;
        struct scsi_cdb_request_sense cdb_request_sense;
        struct scsi_cdb_read_10 cdb_read_10;
    };
    void *pvt;
    char backend[8];
    const struct scsiblockfn *fn;
    uint8_t id;
} scsidev_t;


int scsidev__init(scsidev_t *dev, const struct scsiblockfn *, void *pvt);

int scsidev__inquiry(scsidev_t *dev, uint8_t *status, struct scsi_sbc_inquiry_data *data);

int scsidev__read(scsidev_t *dev, uint8_t *target, sector_t sector, unsigned size, uint8_t *status);
int scsidev__write(scsidev_t *dev, const uint8_t *source, sector_t sector, unsigned size, uint8_t *status);
void scsidev__getname(scsidev_t *dev, char *out);

#endif
