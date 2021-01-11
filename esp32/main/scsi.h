#ifndef __SCSI_H__
#define __SCSI_H__

#include <inttypes.h>
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

struct scsi_cdb_request_sense {
    uint8_t opcode;
    uint8_t desc:1;
    uint8_t rsvd:7;
    le_uint16_t rsvd2;
    uint8_t length;
    uint8_t control;
} __attribute__((packed));

struct scsi_cdb_inquiry {
    uint8_t opcode;
    uint8_t evpd:1;
    uint8_t rsvd1:7;
    uint8_t page;
    be_uint16_t length;
    uint8_t control;
} __attribute__((packed));

struct scsi_cdb_test_unit_ready {
    uint8_t opcode;
    uint8_t rsvd[4];
    uint8_t control;
} __attribute__((packed));

struct scsi_cdb_read_capacity_16 {
    uint8_t opcode;
    uint8_t service_action:5;
    uint8_t rsvd1:3;
    uint8_t lba[8];
    le_uint32_t length;
    uint8_t pmi:1;
    uint8_t rsvd2:7;
    uint8_t control;
} __attribute__((packed));


struct scsi_cdb_read_capacity_10 {
    uint8_t opcode;
    uint8_t service_action:5;
    uint8_t rsvd1:3;
    uint8_t lba[8];
    le_uint32_t length;
    uint8_t pmi:1;
    uint8_t rsvd2:7;
    uint8_t control;
} __attribute__((packed));

struct scsi_cdb_read_10 {
    uint8_t opcode;
    uint8_t obsolete:2;
    uint8_t rarc:1;
    uint8_t fua:1;
    uint8_t dpo:1;
    uint8_t rdprotect:3;
    be_uint32_t lba;
    uint8_t group:5;
    uint8_t rsvd1:3;
    be_uint16_t length;
    uint8_t control;
} __attribute__((packed));


struct scsi_sbc_inquiry_data
{
    uint8_t device_type:5;
    uint8_t peripheral_qualifier: 3;
    uint8_t rsvd1:7;
    uint8_t removable:1;
    uint8_t version;
    uint8_t response_format:4;
    uint8_t hisup:1;
    uint8_t normaca:1;
    uint8_t obsolete1:1;
    uint8_t obsolete2:1;
    uint8_t additional_length;


    uint8_t protect:1;
    uint8_t rsvd2:2;
    uint8_t _3pc:1;
    uint8_t tpgs:2;
    uint8_t acc:1;
    uint8_t sccs:1;

    uint8_t obsolete3:4;
    uint8_t multip:1;
    uint8_t _v5:1;
    uint8_t encserv:1;
    uint8_t obsolete4:1;

    uint8_t v5:1;
    uint8_t cmdque:1;
    uint8_t obsolete5:6;
    uint8_t vendor_id[8];

    uint8_t product_id[16];

    uint8_t product_revision[4];
} __attribute__((packed));

struct scsi_sbc_sense_data
{
    uint8_t code:7;
    uint8_t rsvd1:1;

    uint8_t sense_key:4;
    uint8_t rsvd2:4;

    uint8_t additional_sense_code;
    uint8_t additional_sense_code_qual;
    uint8_t rsvd3[3];
    uint8_t additional_sense_len;
    uint8_t sense_data[10];
}__attribute__((packed));

struct scsi_sbc_read_capacity
{
    be_uint32_t lba;
    be_uint32_t blocksize;
} __attribute__((packed));


#define SBC_CMD_TEST_UNIT_READY                   (0x00)
#define SBC_CMD_REQUEST_SENSE                     (0x03)
#define SBC_CMD_FORMAT_UNIT                       (0x04)
#define SBC_CMD_READ_6                            (0x08)
#define SBC_CMD_INQUIRY                           (0x12)
#define SBC_CMD_MODE_SELECT_6                     (0x15)
#define SBC_CMD_MODE_SENSE_6                      (0x1A)
#define SBC_CMD_START_STOP_UNIT                   (0x1B)
#define SBC_CMD_RECEIVE_DIAGNOSTICS               (0x1C)
#define SBC_CMD_SEND_DIAGNOSTIC                   (0x1D)
#define SBC_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL      (0x1E)
#define SBC_CMD_READ_LONG                         (0x23)
#define SBC_CMD_READ_CAPACITY                     (0x25)
#define SBC_CMD_READ_CD_ROM_CAPACITY              (0x25)
#define SBC_CMD_READ_10                           (0x28)
#define SBC_CMD_WRITE_10                          (0x2A)
#define SBC_CMD_VERIFY_10                         (0x2F)
#define SBC_CMD_SYNCHRONIZE_CACHE                 (0x35)
#define SBC_CMD_WRITE_BUFFER                      (0x3B)
#define SBC_CMD_CHANGE_DEFINITION                 (0x40)
#define SBC_CMD_READ_TOC                          (0x43)
#define SBC_CMD_MODE_SELECT_10                    (0x55)
#define SBC_CMD_RESERVE_10                        (0x56)
#define SBC_CMD_RELEASE_10                        (0x57)
#define SBC_CMD_MODE_SENSE_10                     (0x5A)


#ifdef __cplusplus
}
#endif

#endif

