#include <inttypes.h>
#include <string.h>
#include "tar.h"
#include "strtoint.h"
#include "esp_log.h"
#include "json.h"
#include "firmware.h"
#include "fpga.h"
#include "flash_pgm.h"
#include "ota.h"
#include "rle.h"

#define TAG "Firmware"

#define ALIGN(x, blocksize) ((((uint32_t)(x)+(blocksize-1)) & ~(blocksize-1)))

struct tar_header {
    char name[100];
    uint8_t mode[8];
    uint8_t uid[8];
    uint8_t gid[8];
    uint8_t size[12];
    uint8_t mtime[12];
    uint8_t chksum[8];
    uint8_t typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

static int firmware__validate_tar_header(firmware_upgrade_t *f)
{
    int r = -1;
    struct tar_header *hdr = (struct tar_header*)f->buffer;

    if (strncmp(hdr->magic, TMAGIC, 5)==0) {
        if ((hdr->typeflag == REGTYPE) ||
            (hdr->typeflag == AREGTYPE) ||
            (hdr->typeflag == CONTTYPE))  {
            r = 0;
        } else {
            ESP_LOGE(TAG,"Invalid file type 0x%02x", hdr->typeflag);

        }
    } else {
        ESP_LOGE(TAG,"Invalid TAR magic %02x%02x%02x%02x%02x%02x",
                 hdr->magic[0],
                 hdr->magic[1],
                 hdr->magic[2],
                 hdr->magic[3],
                 hdr->magic[4],
                 hdr->magic[5]);
    }
    return r;
}

static int firmware__read_block_target(firmware_upgrade_t *f, uint8_t *target)
{
    unsigned ptr = 0;

    do {
        if (ptr<sizeof(struct tar_header)) {
            int remain =  sizeof(struct tar_header) - ptr;
            int read = f->readfun( f->readfundata, &target[ptr], remain);
            if (read<=0) {
                ESP_LOGE(TAG,"Short read, requested %d got %d", remain, read);
                return -1;
            }
            ptr += read;
            if (ptr==sizeof(struct tar_header)) {
                ptr = 0;
                return 0;
            }
        } else {
            ptr = 0;
            return 0;
        }
    } while (1);
}

static int firmware__read_block(firmware_upgrade_t *f)
{
    return firmware__read_block_target(f, f->buffer);
}

static int firmware__read_block_header(firmware_upgrade_t *f)
{
    if (firmware__read_block(f)<0)
        return -1;

    if (firmware__validate_tar_header(f)<0) {
        return -1;
    }
    return 0;
}

static int firmware__read_manifest_hdr(firmware_upgrade_t *f)
{
    char size[13];
    int size_int;

    struct tar_header *hdr = (struct tar_header*)f->buffer;

    if (firmware__read_block_header(f)<0) {
        ESP_LOGE(TAG, "Cannot read block header for manifest");
        return -1;
    }

    // Check if it's a manifest.
    if (strcasecmp("manifest.json", hdr->name)==0) {
        // Get manifest size
        memcpy(size, hdr->size, 12);
        size[12] = '\0';
        if (strtoint_octal(size, &size_int)<0) {
            ESP_LOGE(TAG,"Invalid size in TAR header");
            return -1;
        }
        if (size_int > 4093) { // Limit size of manifest
            ESP_LOGE(TAG,"Size too long for manifest: %d %s", size_int, size);
            return -1;
        }
        f->size = size_int;
        f->state = WAIT_MANIFEST_CONTENT;
        f->dynbuf = malloc(ALIGN(size_int, 512));
        return 0;
    }
    ESP_LOGE(TAG, "First file not manifest");
    return -1;
}

static int firmware__read_file_contents(firmware_upgrade_t *f)
{
    int size = f->size;
    uint8_t *target = f->dynbuf;

    while (size>0) {
        ESP_LOGI(TAG, "Reading file block");
        if (firmware__read_block_target(f, target)<0) {
            return -1;
        }
        size-=512;
        target+=512;
    }
    return 0;
}

struct manifest_file_info *firmware__get_file_info(firmware_upgrade_t *f, const char *filename)
{
    int i;

    for (i=0; i<f->filecount; i++) {
        if (strcasecmp(filename,f->fileinfo[i].name)==0) {
            return &f->fileinfo[i];
        }
    }
    return NULL;
}

static int firmware__parse_type(const char *text, firmware_type_t *type)
{
    if (strcmp(text,"ota")==0) {
        *type = FIRMWARE_TYPE_OTA;
    } else if (strcmp(text,"resources")==0) {
        *type = FIRMWARE_TYPE_RESOURCES;
    } else if (strcmp(text,"fpga")==0) {
        *type = FIRMWARE_TYPE_FPGA;
    } else if (strcmp(text,"rom")==0) {
        *type = FIRMWARE_TYPE_ROM;
    } else {
        ESP_LOGE(TAG,"Unknown firmware type '%s'", text);
        return -1;
    }
    return 0;
}

static int firmware__parse_compress(const char *text, firmware_compress_t *compress)
{
    if (strcmp(text,"none")==0) {
        *compress = FIRMWARE_COMPRESS_NONE;
    } else if (strcmp(text,"rle")==0) {
        *compress = FIRMWARE_COMPRESS_RLE;
    } else {
        ESP_LOGE(TAG,"Unknown compression type '%s'", text);
        return -1;
    }
    return 0;
}

static int firmware__check_manifest(firmware_upgrade_t *f)
{
    cJSON *update = cJSON_GetObjectItemCaseSensitive(f->manifest, "update");

    if (NULL==update) {
        ESP_LOGE(TAG,"Invalid JSON");
        return -1;
    }
    const char *version = json__get_string(update, "version");
    //const char *compat = json__get_string(f->manifest, "compat");

    if ((!version) || strcmp(version,"1.0")!=0) {
        ESP_LOGE(TAG,"Invalid version '%s' in manifest", version);
        return -1;
    }

    // Build entry array to undestand which files have been processed.

    int i;

    cJSON *a = cJSON_GetObjectItemCaseSensitive(update, "files");

    if (!cJSON_IsArray(a))
        return -1;

    int num = cJSON_GetArraySize(a);

    f->fileinfo = malloc( num * sizeof(struct manifest_file_info) );

    for (i=0; i<num; i++) {
        cJSON *entry = cJSON_GetArrayItem(a, i);
        const char *file = json__get_string(entry,"name");
        const char *type = json__get_string(entry,"type");
        const char *compress = json__get_string(entry,"compress");

        // TODO: parse type + compress
        if (firmware__parse_type(type, &f->fileinfo[i].type)<0) {
            return -1;
        }
        if (firmware__parse_compress(compress, &f->fileinfo[i].compress)<0) {
            return -1;
        }
        f->fileinfo[i].name = file;
        f->fileinfo[i].completed = false;
    }

    f->filecount = num;

    return 0;
}

static int firmware__read_manifest_content(firmware_upgrade_t *f)
{
    if (firmware__read_file_contents(f)<0) {
        ESP_LOGE(TAG,"Cannot read manifest content");
        return -1;
    }

    ESP_LOGI(TAG,"Reading manifest len %d", f->size);

    // NULL-terminate JSON.
    f->dynbuf[ f->size ] = '\0';

    cJSON *root = cJSON_Parse((char*)f->dynbuf);

    if (!root) {
        ESP_LOGI(TAG, "Error parsing JSON");
        return -1;
    }

    f->manifest = root;

    if (firmware__check_manifest(f)<0) {

        return -1;
    }

    f->state = WAIT_FILES;

    return 0;
}

typedef int (*firmware_streamer_t)(void *, const uint8_t *, unsigned int);

static int firmware__stream_file(firmware_upgrade_t *f,
                                 firmware_compress_t compression,
                                 firmware_streamer_t streamer,
                                 void *streamerdata)
{
    int size = f->size;
    int r;

    if (compression == FIRMWARE_COMPRESS_NONE) {
        while (size>0) {
            int read = f->readfun(f->readfundata, f->buffer, 512);
            if (read<=0) {
                ESP_LOGE(TAG,"Short read on fpga file");
                return -1;
            }
            streamer(streamerdata, f->buffer, read > size ? size: read);
            size-=read;
        }
    } else {
        // RLE compression.

        r = rle_decompress_stream(f->readfun,
                                  f->readfundata,
                                  streamer,
                                  streamerdata,
                                  size);
        if (r<0)
            return r;
        // Read rest of TAR block.
        int partial_read = size % 512;
        if (partial_read>0) {
            ESP_LOGI(TAG, "Reading remaining %d", 512-partial_read);

            if (f->readfun(f->readfundata, f->buffer, 512-partial_read)<0)
                return -1;
        }

    }
    return 0;
}

static int firmware__program_fpga(firmware_upgrade_t *f, firmware_compress_t compress)
{
    flash_program_handle_t handle;

    if (flash_pgm__prepare_programming(&handle,
                                       0x40,
                                       0x00,
                                       NULL,
                                       f->size)<0) {
        ESP_LOGE(TAG,"Cannot start FPGA programming");
        return -1;
    }

    return firmware__stream_file(f, compress, (firmware_streamer_t)&flash_pgm__program_chunk, &handle);
}

static int firmware__program_rom(firmware_upgrade_t *f, firmware_compress_t compress)
{
    flash_program_handle_t handle;

    if (flash_pgm__prepare_programming(&handle,
                                       0x42,
                                       0x00,
                                       NULL,
                                       f->size)<0) {
        ESP_LOGE(TAG,"Cannot start ROM programming");
        return -1;
    }

    return firmware__stream_file(f, compress, (firmware_streamer_t)&flash_pgm__program_chunk, &handle);
}

static int firmware__program_resources(firmware_upgrade_t *f, firmware_compress_t compress)
{
    flash_program_handle_t handle;

    if (flash_pgm__prepare_programming(&handle,
                                       ESP_PARTITION_TYPE_DATA,
                                       ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
                                       "resources",
                                       f->size)<0) {
        ESP_LOGE(TAG,"Cannot start ROM programming");
        return -1;
    }

    return firmware__stream_file(f, compress, (firmware_streamer_t)&flash_pgm__program_chunk, &handle);
}

static int firmware__ota(firmware_upgrade_t *f, firmware_compress_t compress)
{
    ota_stream_handle_t handle;

    if (ota__init(&handle)<0)
        return -1;

    if (ota__start(&handle, f->size)<0) {
        ESP_LOGE(TAG,"Cannot start OTA programming");
        return -1;
    }

    return firmware__stream_file(f, compress, (firmware_streamer_t)&ota__chunk, &handle);
}

static int firmware__read_file_entry(firmware_upgrade_t *f)
{
    char size[13];
    int size_int;

    struct tar_header *hdr = (struct tar_header*)f->buffer;

    if (firmware__read_block_header(f)<0) {
        ESP_LOGE(TAG, "Cannot read block header for file entry");
        return -1;
    }

    struct manifest_file_info *info = firmware__get_file_info(f, hdr->name);

    if (info==NULL) {
        ESP_LOGE(TAG,"Cannot find firmware information for '%s'", hdr->name);
        return -1;
    }

    if (info->completed) {
        ESP_LOGE(TAG, "Firmware '%s' already programmed", hdr->name);
        return -1;
    }

    memcpy(size, hdr->size, 12);
    size[12] = '\0';
    if (strtoint_octal(size, &size_int)<0) {
        ESP_LOGE(TAG,"Invalid size in TAR header");
        return -1;
    }

    f->size = size_int;

    int r = -1;

    switch (info->type) {
    case FIRMWARE_TYPE_OTA:
        ESP_LOGI(TAG, "OTA Firmware");
        r = firmware__ota(f, info->compress);
        if (r==0) {
            ESP_LOGI(TAG, "OTA programming completed");
            info->completed = true;
        }
        break;
    case FIRMWARE_TYPE_FPGA:
        r = firmware__program_fpga(f, info->compress);
        if (r==0) {
            ESP_LOGI(TAG, "FPGA programming completed");
            info->completed = true;
        }
        break;
    case FIRMWARE_TYPE_RESOURCES:
        ESP_LOGI(TAG, "Resources Firmware");
        r = firmware__program_resources(f, info->compress);
        if (r==0) {
            ESP_LOGI(TAG, "Resources programming completed");
            info->completed = true;
        }
        break;

    case FIRMWARE_TYPE_ROM:
        ESP_LOGI(TAG, "ROM Firmware");
        r = firmware__program_rom(f, info->compress);
        if (r==0) {
            ESP_LOGI(TAG, "ROM programming completed");
            info->completed = true;
        }
        break;
    }

    return r;
}

static bool firmware__all_completed(firmware_upgrade_t *f)
{
    int i;
    bool result = true;
    for (i=0;i<f->filecount;i++) {
        if (f->fileinfo[i].completed==false) {
            result=false;
            break;
        }
    }
    return result;
}

int firmware__upgrade(firmware_upgrade_t *f)
{
    int r = -1;
    ESP_LOGI(TAG,"Starting firmware upgrade");

    while (1) {
        ESP_LOGI(TAG,"State: %d", f->state);
        switch(f->state) {
        case WAIT_MANIFEST_HDR:
            r = firmware__read_manifest_hdr(f);
            break;
        case WAIT_MANIFEST_CONTENT:
            r = firmware__read_manifest_content(f);
            break;
        case WAIT_FILES:
            if (firmware__all_completed(f)) {
                ESP_LOGI(TAG, "All programming files processed.");
                return 0;
            }
            ESP_LOGI(TAG, "Reading programming file");
            r = firmware__read_file_entry(f);
            break;
        default:
            break;
        }
        if (r<0)  {
            ESP_LOGE(TAG, "Error occurred, aborting firmware upgrade");
            f->state = ERROR;
            break;
        }
    }
    return r;
}

void firmware__init(firmware_upgrade_t *f,
                    firmware_read_fun fun,
                    void *data)
{
    f->state = WAIT_MANIFEST_HDR;
    f->readfun = fun;
    f->readfundata = data;
    f->dynbuf = NULL;
    f->manifest = NULL;
    f->fileinfo = NULL;
}

void firmware__deinit(firmware_upgrade_t *f)
{
    if (f->dynbuf)
        free(f->dynbuf);
    f->dynbuf = NULL;
    if (f->manifest) {
        cJSON_free(f->manifest);
        f->manifest = NULL;
    }
    if (f->fileinfo) {
        free(f->fileinfo);
        f->fileinfo = NULL;
    }
}
