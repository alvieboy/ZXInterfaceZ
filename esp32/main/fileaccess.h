#ifndef __FILEACCESS_H__
#define __FILEACCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "strcmpi.h"
#include <esp_vfs.h>
#include <esp_spiffs.h>
#include "sdkconfig.h"
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

    /*
#define FILE_FILTER_NONE (0x00)
#define FILE_FILTER_SNA (0x01)
#define FILE_FILTER_TAP (0x02)

#define FILE_FILTER_TAPES (0x02)
#define FILE_FILTER_ROMS (0x03)
#define FILE_FILTER_POKES (0x04)
      */
#define FILE_PATH_MAX (128 + CONFIG_SPIFFS_OBJ_NAME_LEN)

#ifdef __linux__
extern char startupdir[];
#endif

int __chdir(const char *dir);
DIR *__opendir(const char *path);
struct dirent *__readdir(DIR*dir);

const char *__getcwd_const(void);
char *__getcwd(char *dest, int maxlen);
int __in_rootdir(void);


static inline const char *get_file_extension(const char *src)
{
    const char *delim = strrchr(src,'.');
    if (!delim) {
        return NULL;
    }
    delim++;
    return delim;
}

static inline bool ext_match(const char *fileext, const char *reqext)
{
    return strcmpi(fileext,reqext)==0;
}

#define MAX_MOUNTPOINTS 4

struct mountpoints {
    int count;
    const char *mounts[MAX_MOUNTPOINTS];
};

char *fullpath(const char *name, char *dest, int maxlen);
int file_size(const char *path, const char *filename);
int __open(const char *path, int flags, ...);
int __lstat(const char *path, struct stat *st);
void *readfile(const char *path, int *size);
FILE *__fopen(const char *path, const char *mode);


const struct mountpoints *__get_mountpoints(void);
void register_mountpoint(const char *path);
void unregister_mountpoint(const char *path);


typedef enum {
    TYPE_INVALID,
    TYPE_FILE,
    TYPE_DIRECTORY
} filetype_t;

filetype_t file_type(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
