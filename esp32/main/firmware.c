#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#include "stream.h"
#include "board.h"

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

struct flash_wrapper {
    firmware_upgrade_t *f;
    flash_program_handle_t handle;
    struct manifest_file_info *info;
};

struct ota_wrapper {
    firmware_upgrade_t *f;
    ota_stream_handle_t handle;
    struct manifest_file_info *info;
};

static unsigned firmware__file_progress_percent(firmware_upgrade_t*f);
static unsigned firmware__overall_progress_percent(firmware_upgrade_t*f);

#define NO_PERCENT_UPDATE (-1)

static int firmware__progress_start(firmware_upgrade_t *f)
{
    if (f->progress_reporter)
    {
        f->progress_reporter->start(f->progress_reporter_data);
    }
    return 0;
}

static int firmware__progress_report(firmware_upgrade_t *f, int percent)
{
    if (f->progress_reporter)
    {
        f->progress_reporter->report(f->progress_reporter_data, percent);
    }
    return 0;
}

static int firmware__progress_report_action(firmware_upgrade_t *f, int percent, progress_level_t level, const char *fmt, ...)
{
    va_list ap;
    if (f->progress_reporter)
    {
        char text[128];
        va_start(ap, fmt);
        vsnprintf(text, sizeof(text), fmt, ap);
        f->progress_reporter->report_action(f->progress_reporter_data, percent, level, text);
        va_end(ap);
    }
    return 0;
}

static int firmware__progress_report_phase_action(firmware_upgrade_t *f, int percent, const char *phase, progress_level_t level, const char *fmt, ...)
{
    va_list ap;
    if (f->progress_reporter)
    {
        char text[128];
        va_start(ap, fmt);
        vsnprintf(text, sizeof(text), fmt, ap);
        f->progress_reporter->report_phase_action(f->progress_reporter_data, percent, phase, level, text);
        va_end(ap);
    }
    return 0;
}

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
    int read = stream__read_blocking( f->stream, target, sizeof(struct tar_header));

    if (read==sizeof(struct tar_header))
        return 0;

    ESP_LOGE(TAG,"Short read, requested %d got %d", sizeof(struct tar_header), read);
    return -1;
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
        f->dynbuf = malloc(ALIGN((unsigned)size_int, 512));
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

static struct manifest_file_info *firmware__get_file_info(firmware_upgrade_t *f, const char *filename)
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

static int parsehexnibble(const char p)
{
    if (p>='0' && p<='9')
        return (int)(p-'0');
    if (p>='a' && p<='f')
        return (int)(p-'a'+10);
    if (p>='A' && p<='F')
        return (int)(p-'A'+10);
    return -1;
}

static int parsehexbyte(const char *ptr)
{
    int v1,v2;
    v1 = parsehexnibble(*ptr++);
    if (v1<0)
        return v1;
    v2 = parsehexnibble(*ptr++);
    if (v2<0)
        return v2;
    return (v1<<4) + v2;
}

static int firmware__parse_sha(const char *text, uint8_t shabin[32])
{
    uint8_t *store = &shabin[0];

    if (strlen(text)!=64)
        return -1;
    int i;
    for (i=0;i<32; i++) {
        int value = parsehexbyte(text);
        if (value<0)
            return -1;
        text+=2;
        *store++ = (uint8_t)value;
    }
    return 0;
}

static void firmware__print_sha(const uint8_t *sha, char *dest)
{
    for (int i=0;i<32;i++) {
        dest+=sprintf(dest, "%02x", *sha++);
    }
}

static int firmware__check_compatibility(const char *compat)
{
    char *toks[8];     // Up to 8 board revisions

    bool valid = false;
    // Duplicate string (in stack) so we can tokenize it
    char *dcompat = strdupa(compat);
    int i = 0;
    toks[i++] = strtok(dcompat, ",");

    while (i<8) {
        toks[i]=strtok(NULL,",");
        if (toks[i]==NULL)
            break;
        i++;
    }

    // Process them one at the time.

    for (int x=0;x<i;x++) {
        char *brev = toks[x];
        uint8_t minor, major;

        if (board__parseRevision(brev, &major, &minor)<0) {
            ESP_LOGE(TAG,"Invalid board revision '%s'", toks[x]);
            return -1;
        }

        if (board__isCompatible(major,minor)) {
            valid = true;
        }
    }
    if (!valid)
        return -1;
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
    const char *compat = json__get_string(update, "compat");

    if (version==NULL || compat==NULL) {
        return -1;
    }

    if ((!version) || strcmp(version,"1.1")!=0) {
        ESP_LOGE(TAG,"Invalid version '%s' in manifest", version);
        return -1;
    }
    // Build entry array to undestand which files have been processed.

    // Check compatibility
    if (firmware__check_compatibility(compat)<0) {
        ESP_LOGE(TAG,"Incompatible version");
        return -1;
    }

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
        int size = json__get_integer_default(entry,"size",-1);
        if (size<0) {
            return -1;
        }
        if (file==NULL || type==NULL || compress==NULL) {
            return -1;
        }
        // TODO: parse type + compress
        if (firmware__parse_type(type, &f->fileinfo[i].type)<0) {
            return -1;
        }
        if (firmware__parse_compress(compress, &f->fileinfo[i].compress)<0) {
            return -1;
        }
        f->fileinfo[i].name = file;
        f->fileinfo[i].completed = false;
        f->fileinfo[i].processed_size = 0;
        f->fileinfo[i].size = (unsigned)size;
        // Process SHAs
        const char *sha = json__get_string(entry,"sha256");
        if (sha) {
            if (firmware__parse_sha(sha, &f->fileinfo[i].sha[0] )<0) {
                ESP_LOGE(TAG,"Invalid SHA field");
                return -1;
            }
        } else {
            ESP_LOGE(TAG,"Missing required SHA field");
            return -1;
        }
#ifdef ENABLE_COMPRESS_SHA
        /* we don't actually need compressed SHA, because we do validate after decompression.
         Still it might be useful to debug the decompression itself */
        if (f->fileinfo[i].compress != FIRMWARE_COMPRESS_NONE) {
            // Get compressed SHA
            const char *csha = json__get_string(entry,"compressed_sha256");
            if (csha) {
                if (firmware__parse_sha(csha, &f->fileinfo[i].compressed_sha[0] )<0) {
                    ESP_LOGE(TAG,"Invalid compressed SHA field");
                    return -1;
                }
            } else {
                ESP_LOGE(TAG,"Missing required compressed SHA field");
                return -1;
            }
        }
#endif

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
            int read = stream__read(f->stream, f->buffer, 512);
            if (read<=0) {
                ESP_LOGE(TAG,"Short read on stream file");
                return -1;
            }
            ESP_LOGI(TAG,"stream %d %d\n", read, size);
            if (streamer(streamerdata, f->buffer, read > size ? size: read)<0) {
                ESP_LOGE(TAG, "Error streaming firmware data");
                return -1;
            }
            size-=read;
        }
    } else {
        // RLE compression.

        // Enable buffering if we have not already done so.
        ESP_LOGI(TAG,"RLE compressed data, enabling buffering");
        stream__set_buffering(f->stream, true);

        r = rle_decompress_stream(f->stream,
                                  streamer,
                                  streamerdata,
                                  (size_t)size);
        if (r<0)
            return r;
        // Read rest of TAR block.
        int partial_read = size % 512;
        if (partial_read>0) {
            ESP_LOGI(TAG, "Reading remaining %d", 512-partial_read);

            if (stream__read_blocking(f->stream, f->buffer, 512-partial_read)<0)
                return -1;
        }

    }
    return 0;
}

static int firmware__chech_sha(struct manifest_file_info *info, uint8_t *shaResult)
{
    char shatxt[65];
    if (memcmp(info->sha, shaResult, 32)!=0) {
        ESP_LOGE(TAG, "Invalid SHA result!");

        firmware__print_sha(info->sha, shatxt);
        ESP_LOGE(TAG, " Expected: %s", shatxt);

        firmware__print_sha(shaResult, shatxt);
        ESP_LOGE(TAG, " Computed: %s", shatxt);

        return -1;
    }
    firmware__print_sha(info->sha, shatxt);
    ESP_LOGI(TAG, "SHA match: %s", shatxt);
    return 0;
}


static int firmware__flash_chunk_wrapper(struct flash_wrapper *wrap, const uint8_t *data, unsigned len)
{
    int r;
    r = flash_pgm__program_chunk(&wrap->handle, data, len);

    unsigned block4k = wrap->info->processed_size >> 12;

    wrap->info->processed_size += len;

    if (block4k != (wrap->info->processed_size >> 12)) {
        firmware__progress_report( wrap->f, firmware__overall_progress_percent(wrap->f) );
    }

    if (r==0) {
        ESP_LOGI(TAG, "Program chunk %d",len);
        mbedtls_md_update(&wrap->f->shactx, data, len);
    } 

    return r;
}

static int firmware__program_resources(firmware_upgrade_t *f, struct manifest_file_info *info)
{
    uint8_t shaResult[32];
    struct flash_wrapper wrap;
    int r;

    if (flash_pgm__prepare_programming(&wrap.handle,
                                       ESP_PARTITION_TYPE_DATA,
                                       ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
                                       "resources",
                                       f->size)<0) {
        ESP_LOGE(TAG,"Cannot start ROM programming");
        return -1;
    }

    // prepare SHA

    mbedtls_md_init(&f->shactx);
    mbedtls_md_setup(&f->shactx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&f->shactx);

    wrap.f = f;
    wrap.info = info;

    r = firmware__stream_file(f, info->compress, (firmware_streamer_t)&firmware__flash_chunk_wrapper, &wrap);

    mbedtls_md_finish(&f->shactx, shaResult);
    mbedtls_md_free(&f->shactx);

    
    if (r==0) {
        r = firmware__chech_sha(info, shaResult);
        if (r<0) {
            flash_pgm__abort(&wrap.handle);
        } else {
            // Call flush even if we errored, but keep error result
            r = flash_pgm__flush(&wrap.handle);
            if (r<0) {
                flash_pgm__abort(&wrap.handle);
            }
        }
    } else {
        flash_pgm__abort(&wrap.handle);
    }

    return r;
}

static int firmware__ota_chunk_wrapper(struct ota_wrapper *wrap, const uint8_t *data, unsigned len)
{
    int r;
    r = ota__chunk(&wrap->handle, data, len);

    unsigned block4k = wrap->info->processed_size >> 12;

    wrap->info->processed_size += len;

    if (block4k != (wrap->info->processed_size >> 12)) {
        firmware__progress_report( wrap->f, firmware__overall_progress_percent(wrap->f) );
    }

    if (r>=0) {
        mbedtls_md_update(&wrap->f->shactx, data, len);
    }

    return r;
}

static int firmware__ota(firmware_upgrade_t *f, struct manifest_file_info *info)
{
    uint8_t shaResult[32];
    struct ota_wrapper wrap;

    wrap.f = f;
    wrap.info = info;

    if (ota__init(&wrap.handle)<0)
        return -1;

    if (ota__start(&wrap.handle, f->size)<0) {
        ESP_LOGE(TAG,"Cannot start OTA programming");
        return -1;
    }

    mbedtls_md_init(&f->shactx);
    mbedtls_md_setup(&f->shactx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&f->shactx);

    int r = firmware__stream_file(f, info->compress, (firmware_streamer_t)&firmware__ota_chunk_wrapper, &wrap);

    mbedtls_md_finish(&f->shactx, shaResult);
    mbedtls_md_free(&f->shactx);
    if (r==0) {
        r = firmware__chech_sha(info, shaResult);
        if (r==0) {
            r = ota__finish(&wrap.handle);
        } else {
            ota__abort(&wrap.handle);
            r=-1;
        }
    } else {
        ota__abort(&wrap.handle);
    }

    return r;
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
    // Sanity - compare size with manifest size
    if (info->size != f->size) {
        if (!info->compress!=FIRMWARE_COMPRESS_NONE) {
            ESP_LOGI(TAG, "Size mismatch for uncompressed file: file %d manifest %d", f->size, info->size);
            return -1;
        }
    }
    ESP_LOGI(TAG, "Firmware size %d uncompressed %d\n", f->size, info->size);

    int r = -1;

    const char *phase = NULL;
    switch (info->type) {
    case FIRMWARE_TYPE_OTA:
        phase = "Programming OTA executable firmware";
        break;
    case FIRMWARE_TYPE_RESOURCES:
        phase = "Programming resources";
        break;
    }

    if (phase) {
        firmware__progress_report_phase_action(f,
                                               firmware__overall_progress_percent(f),
                                               phase,
                                               PROGRESS_LEVEL_INFO,
                                               "Applying firmware changes");
    }


    switch (info->type) {
    case FIRMWARE_TYPE_OTA:
        ESP_LOGI(TAG, "OTA Firmware");
        r = firmware__ota(f, info);
        if (r==0) {
            ESP_LOGI(TAG, "OTA programming completed");
            info->completed = true;
        }
        break;
    case FIRMWARE_TYPE_RESOURCES:
        ESP_LOGI(TAG, "Resources Firmware");
        r = firmware__program_resources(f, info);
        if (r==0) {
            ESP_LOGI(TAG, "Resources programming completed");
            info->completed = true;
        }
        break;
    }

    return r;
}

static unsigned firmware__get_progress(struct manifest_file_info *info)
{
    return (info->processed_size*100) / info->size;
}

static unsigned firmware__file_progress_percent(firmware_upgrade_t*f)
{
    unsigned total = 0;
    bool incomplete = false;
    int i;
    for (i=0;i<f->filecount;i++) {
        if (!f->fileinfo[i].completed)
            incomplete=true;
        total += firmware__get_progress(&f->fileinfo[i]);
    }

    total/=(unsigned)f->filecount;

    if (total>99)
        total=99;

    if (!incomplete)
        total=100;

    return total;
}

static unsigned firmware__overall_progress_percent(firmware_upgrade_t*f)
{
    unsigned progress = 0;

    switch(f->state) {
    case WAIT_MANIFEST_HDR:
        progress = 0;
        break;
    case WAIT_MANIFEST_CONTENT:
        progress = 5;
        break;
    case WAIT_FILES:
        // Up to 99 only;
        progress = firmware__file_progress_percent(f);

        progress*=3678;
        progress/=4096; // Mutiply by 0.9

        progress+=10;

        break;
    case ERROR:
        progress = 100;
    }
    return progress;
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

    firmware__progress_start(f);

    while (1) {
        ESP_LOGI(TAG,"State: %d", f->state);
        switch(f->state) {
        case WAIT_MANIFEST_HDR:
            r = firmware__read_manifest_hdr(f);
            firmware__progress_report_phase_action(f, firmware__overall_progress_percent(f), "Processing manifest", PROGRESS_LEVEL_INFO, "Reading firmware manifest");
            break;
        case WAIT_MANIFEST_CONTENT:
            r = firmware__read_manifest_content(f);
            firmware__progress_report_phase_action(f, firmware__overall_progress_percent(f), "Processing manifest", PROGRESS_LEVEL_INFO, "Analysing firmware components");
            break;
        case WAIT_FILES:
            if (firmware__all_completed(f)) {
                ESP_LOGI(TAG, "All programming files processed.");

                firmware__progress_report_phase_action(f,
                                                       100,
                                                       "Upgrade completed",
                                                       PROGRESS_LEVEL_INFO,
                                                       "Completed, all sections programmed.");

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
                    struct stream *stream)
{
    f->state = WAIT_MANIFEST_HDR;
    f->stream = stream;
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

void firmware__set_reporter(firmware_upgrade_t *f, const progress_reporter_t *reporter, void *userdata)
{
    f->progress_reporter_data = userdata;
    f->progress_reporter = reporter;
}
