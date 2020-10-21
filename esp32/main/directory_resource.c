#include "directory_resource.h"
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include "fpga.h"
#include "fileaccess.h"
#include "strcmpi.h"
#include "dump.h"
#include "list.h"

static bool filter_match(struct directory_resource *res, struct dirent *e)
{
    const char *ext;
    if (e->d_type == DT_DIR) {
        return true;
    } else {
        if (res->filter==FILE_FILTER_NONE)
            return true;

        ext = get_file_extension(e->d_name);
        switch(res->filter) {
        case FILE_FILTER_SNAPSHOTS:
            return ext_match(ext, "sna") || ext_match(ext,"z80");
            break;
        case FILE_FILTER_ROMS:
            return ext_match(ext, "rom");
            break;
        case FILE_FILTER_TAPES:
            return ext_match(ext, "tap") || ext_match(ext,"tzx");
            break;
        default:
            break;
        }
    }
    return false;
}

static void directory_resource__update_from_mountpoints(struct directory_resource*dr)
{
    const struct mountpoints *mp = __get_mountpoints();
    int space_needed = 2; // For root placeholder
    int i;
    unsigned char *bptr;

    for (i=0;i<mp->count;i++) {
        space_needed += 2 + strlen(mp->mounts[i]);
    }

    dr->buffer = malloc(space_needed);
    if (dr->buffer==NULL) {
        ESP_LOGE(TAG,"Cannot allocate memory");
        return;
    }
    bptr = dr->buffer;
    *bptr++='/';
    *bptr++='\0';
    for (i=0;i<mp->count;i++) {
        *bptr++=0x01; // Type

        int dlen = strlen(mp->mounts[i]);
        memcpy(bptr, mp->mounts[i], dlen+1);
        bptr += dlen+1;
    }
    dr->entries = mp->count;
    dr->alloc_size = space_needed;
    ESP_LOGI(TAG, "Published %d (root) entries", mp->count);

}

static int directory_resource__compare(void *a, void *b)
{
    int i;

    char *pa = &((char*)a)[1];
    char *pb = &((char*)b)[1];

    uint8_t type_a = ((uint8_t*)a)[0];
    uint8_t type_b = ((uint8_t*)b)[0];

    if (type_a>type_b)
        return 1;
    if (type_a<type_b)
        return -1;
    // Same type

    i = strcmp(pa, pb);
    
    return  0 - i;  // Inverse
}

void directory_resource__update(struct resource *res)
{
    struct directory_resource *dr  = (struct directory_resource *)res;
    char cwd[128];
    struct dirent *ent;
    int space_needed = 0;
    int entries = 0;
    unsigned char *bptr;

    DIR *dir;

    if (dr->buffer) {
        free(dr->buffer);
        dr->buffer = NULL;
    }

    dr->entries = 0;
    if (__in_rootdir()) {
        directory_resource__update_from_mountpoints(dr);
        return;
    }


    if (__getcwd(cwd,sizeof(cwd))==NULL) {
        ESP_LOGE(TAG, "Cannot get root directory!");
        return;
    }

    ESP_LOGI(TAG,"Loading directory '%s'", cwd);


    // TODO: get cwd short name

    char *cdir = strrchr(cwd,'/');
    if (NULL==cdir) {
        ESP_LOGE(TAG, "Cannot extract current dir from '%s'", cwd);
        return;
    }
    if (cdir[1] != '\0') {
        cdir++;
    }
    ESP_LOGI(TAG,"Current visible dir: '%s'", cdir);

    // update size for publising cdir
    int cdir_len = strlen(cdir);
    space_needed += cdir_len + 1;

    // 1st pass: get name lengths
    dir = __opendir(cwd);
    if (NULL==dir) {
        cwd[0] = '/';
        cwd[1] = '\0';
        cdir = cwd;
        dir = __opendir(cwd);
        if (!dir) {
            ESP_LOGI(TAG,"Cannot open dir: '%s'", strerror(errno));
            return;
        }
    }
    while ((ent=__readdir(dir))) {
        if (filter_match(dr, ent)) {
            space_needed += 2; // One byte flags, one byte len
            space_needed += strlen(ent->d_name); // One byte flags, one byte len
            entries++;
            if (entries>255)
                break; // Limit 255
        }
    }

    rewinddir(dir);

    // we need to add "..". That's 4 bytes.
    space_needed += 4;

    dr->buffer = malloc(space_needed);
    if (dr->buffer==NULL) {
        ESP_LOGE(TAG,"Cannot allocate memory");
        closedir(dir);
        return;
    }
    bptr = dr->buffer;

    // Put cdir
    memcpy(bptr, cdir, cdir_len);
    // Move past it
    bptr += cdir_len;
    *bptr++ = '\0';

    // Add "..";
    *bptr++=0x01; // Directory
    *bptr++='.'; 
    *bptr++='.'; 
    *bptr++=0x00;

    entries++;

    /* Load entries and sort accordingly */

    char *chbuf = (char*)malloc(space_needed);
    char *chptr = chbuf;

    if (chbuf==NULL)
        return;

    dlist_t *list = NULL;

    while ((ent=__readdir(dir))) {
        if (filter_match(dr, ent)) {
            uint8_t type;
            int dlen;
            char *this_element = chptr;

            if (ent->d_type==DT_DIR) {
                type = 1;
            } else {
                type = 0;
            }
            *chptr++=type;
            dlen = strlen(ent->d_name);
            memcpy(chptr, ent->d_name, dlen+1);
            chptr += dlen+1;

            list = dlist__insert_sorted(list, directory_resource__compare, this_element);
        }
    }
    /* Re-assemble */
    dlist_t *listptr = list;
    while (listptr) {
        uint8_t d;
        uint8_t *data = dlist__data(listptr);
        *bptr++ = *data++;
        // Copy string over
        do {
            d = *data++;
            *bptr++ = d;
        } while (d);

        listptr = dlist__next(listptr);
    }
    free(chbuf);
    dlist__remove_all(list, NULL, NULL);

#if 0
    while ((ent=__readdir(dir))) {
        if (filter_match(dr, ent)) {
            uint8_t type;
            int dlen;

            if (ent->d_type==DT_DIR) {
                type = 1;
            } else {
                type = 0;
            }
            *bptr++=type;
            dlen = strlen(ent->d_name);
            memcpy(bptr, ent->d_name, dlen+1);
            bptr += dlen+1;
        }
    }
#endif
    closedir(dir);
    dr->entries = entries;
    dr->alloc_size = space_needed;
    ESP_LOGI(TAG, "Published %d entries", entries);
}

void directory_resource__set_filter(struct directory_resource *r, uint8_t filter)
{
    r->filter = filter;
}

int directory_resource__sendToFifo(struct resource *res)
{
    struct directory_resource *dr  = (struct directory_resource *)res;
    int r = fpga__load_resource_fifo( &dr->entries, sizeof(dr->entries), RESOURCE_DEFAULT_TIMEOUT);
    if (r<0)
        return -1;
    dump__buffer(dr->buffer, dr->alloc_size);
    r = fpga__load_resource_fifo( dr->buffer, dr->alloc_size, RESOURCE_DEFAULT_TIMEOUT);
    return r;
}

uint8_t directory_resource__type(struct resource *res)
{
    return RESOURCE_TYPE_DIRECTORYLIST;
}

uint16_t directory_resource__len(struct resource *res)
{
    struct directory_resource *dr  = (struct directory_resource *)res;
    return 1 + dr->alloc_size;
}

