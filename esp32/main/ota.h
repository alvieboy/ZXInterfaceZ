#ifndef __OTA_H__
#define __OTA_H__

#include "command.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

typedef struct ota_stream_handle {
    int size;
    int offset;
    esp_ota_handle_t update_handle;
    const esp_partition_t *configured;
    const esp_partition_t *running;
    const esp_partition_t *update_partition;
    bool image_header_was_checked;

    uint8_t header[sizeof(esp_image_header_t) +
                   sizeof(esp_image_segment_header_t) +
                   sizeof(esp_app_desc_t)];
    unsigned headerlen;

} ota_stream_handle_t;

int ota__init(ota_stream_handle_t *);
int ota__performota_cmd(command_t *cmdt, int argc, char **argv);
int ota__start(ota_stream_handle_t *, int size);
int ota__chunk(ota_stream_handle_t *, const uint8_t *data, int len);
int ota__abort(ota_stream_handle_t *h);
int ota__finish(ota_stream_handle_t *h);


#endif
