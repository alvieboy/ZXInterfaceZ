#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <inttypes.h>
#include <cJSON.h>
#include <stdbool.h>

typedef int (*firmware_read_fun)(void *, uint8_t *buffer, size_t len);


typedef enum {
    FIRMWARE_TYPE_OTA,
    FIRMWARE_TYPE_RESOURCES
} firmware_type_t;

typedef enum {
    FIRMWARE_COMPRESS_NONE,
    FIRMWARE_COMPRESS_RLE
} firmware_compress_t;

struct manifest_file_info {
    const char *name;  // Points into JSON object
    bool completed;
    firmware_type_t type;
    firmware_compress_t compress;
};

typedef enum {
    WAIT_MANIFEST_HDR,
    WAIT_MANIFEST_CONTENT,
    WAIT_FILES,
    ERROR
} firmware_state_t;

typedef struct {
    firmware_state_t state;
    int size;
    uint8_t *dynbuf;
    uint8_t filecount;
    uint8_t buffer[512];
    int current_size;
    firmware_read_fun readfun;
    void *readfundata;
    cJSON *manifest;
    struct manifest_file_info *fileinfo;
} firmware_upgrade_t;


void firmware__init(firmware_upgrade_t *f,
                    firmware_read_fun fun,
                    void *data);
void firmware__deinit(firmware_upgrade_t *f);

int firmware__upgrade(firmware_upgrade_t *f);

#endif
