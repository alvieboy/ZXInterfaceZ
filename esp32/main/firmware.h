#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <inttypes.h>
#include <cJSON.h>
#include <stdbool.h>
#include "mbedtls/md.h"
#include "progress.h"

typedef int (*firmware_read_fun)(void *, uint8_t *buffer, size_t len);
struct stream;

typedef enum {
    FIRMWARE_TYPE_OTA,
    FIRMWARE_TYPE_RESOURCES
} firmware_type_t;

typedef enum {
    FIRMWARE_COMPRESS_NONE,
    FIRMWARE_COMPRESS_RLE
} firmware_compress_t;

struct manifest_file_info {
    const char *name;  // Points into JSON object! Do not read this unless the JSON node is still "alive".
    unsigned size;
    unsigned processed_size;
    uint8_t sha[32];
#ifdef ENABLE_COMPRESS_SHA
    uint8_t compressed_sha[32];
#endif
    firmware_type_t type;
    firmware_compress_t compress;
    bool completed;
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
    struct stream *stream;
    cJSON *manifest;
    struct manifest_file_info *fileinfo;
    mbedtls_md_context_t shactx;
#ifdef ENABLE_COMPRESS_SHA
    mbedtls_md_context_t compressed_shactx;
#endif
    const progress_reporter_t *progress_reporter;
    void *progress_reporter_data;
} firmware_upgrade_t;


void firmware__init(firmware_upgrade_t *f,
                    struct stream *);

void firmware__deinit(firmware_upgrade_t *f);

int firmware__upgrade(firmware_upgrade_t *f);

void firmware__set_reporter(firmware_upgrade_t *,const progress_reporter_t *reporter, void *userdata);

#endif
