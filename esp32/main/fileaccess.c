#include <string.h>
#include "esp_log.h"
#include "defs.h"
#include <dirent.h>
#include "fileaccess.h"
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

#define CWD_MAX 255
static char cwd[CWD_MAX];
static uint8_t init = 0;

static void init_cwd()
{
    if(!init) {
        strcpy(cwd,"/");
        init=1;
    }
}

int __in_rootdir()
{
    init_cwd();
    return cwd[0]=='/' && cwd[1]=='\0';
}

int __chdir(const char *dir)
{
    char newwd[CWD_MAX];

    init_cwd();

    strcpy(newwd, cwd);
    int len = strlen(newwd);

    if (strcmp(dir,"..")==0) {
        // Moving backwards
        if (__in_rootdir()) {
            return -1;
        }
        char *lastbl = strrchr(newwd,'/');
        if (!lastbl) {
            return -1; // This cannot happen!
        }
        if (lastbl!=newwd) {
            *lastbl='\0';
        } else {
            // Moved to root.
            lastbl[1] = '\0';
            strcpy(cwd, newwd);
            return 0;
        }

    }

    if (len>1) {
        // Not Root.
        newwd[len++] = '/';
        newwd[len]= '\0';
    }
    strcpy(&newwd[len], dir);

    DIR *d;
#ifdef __linux__
    do {
        char fpath[512];
        sprintf(fpath,"%s/%s", startupdir, newwd);
        ESP_LOGI(TAG, "Chdir to '%s'", fpath);
        d = opendir(fpath);
    } while (0);
#else
    d = opendir(newwd);
#endif
    if (!d) {
        ESP_LOGI(TAG, "Cannot chdir to '%s'", newwd);
        return -1;
    }
    closedir(d);
    strcpy(cwd, newwd);
    return 0;
}

DIR *__opendir(const char *path)
{
    DIR *d;
#ifdef __linux__
    do {
        char fpath[512];
        sprintf(fpath,"%s/%s", startupdir, path);
        ESP_LOGI(TAG, "Chdir to '%s'", fpath);
        d = opendir(fpath);
    } while (0);
#else
    d = opendir(path);
#endif
    return d;
}

int __open(const char *path, int flags, ...)
{
    int fd;
    mode_t mode = 0;
    va_list ap;
    va_start(ap, flags);
    if (flags & O_CREAT) {
        mode = va_arg(ap, mode_t);
    }
#ifdef __linux__
    do {
        char fpath[512];
        sprintf(fpath,"%s/%s", startupdir, path);
        fd = open(fpath, flags, mode);
    } while (0);
#else
    fd = open(path, flags, mode);
#endif
    va_end(ap);
    return fd;
}

FILE *__fopen(const char *path, const char *mode)
{
    FILE *f;
#ifdef __linux__
    do {
        char fpath[512];
        sprintf(fpath,"%s/%s", startupdir, path);
        f = fopen(fpath, mode);
    } while (0);
#else
    f = fopen(path, mode);
#endif
    return f;
}


int __lstat(const char *path, struct stat *st)
{
    int r;
#ifdef __linux__
    do {
        char fpath[512];
        sprintf(fpath,"%s/%s", startupdir, path);
        r = stat(fpath, st);
    } while (0);
#else
    r = stat(path, st);
#endif
    return r;
}

const char *__getcwd_const()
{
    init_cwd();
    return cwd;
}

char *__getcwd(char *dest, int maxlen)
{
    init_cwd();
    strncpy(dest, cwd, maxlen);
    return dest;
}

char *fullpath(const char *name, char *dest, int maxlen)
{
    char *d = dest;
    __getcwd(d, maxlen);
    ESP_LOGI(TAG, "Computing full path cwd='%s', file '%s'",d, name);

    while (d[1]) {
        d++;
        maxlen--;
    }
    if (d[0]!='/') {
        maxlen--;
        if (maxlen==0) {
            ESP_LOGI(TAG, "Overflow");
            return d;
        }
        ESP_LOGI(TAG, "Append slash");

        d++;
        *d = '/';
        d++;
    } else {
        ESP_LOGI(TAG, "Has trailer");
        d++;
    }
    strncpy(d, name, maxlen);
    return d;
}

static const struct {
    struct mountpoints m;
    const char *entries[1];
} mpoints = {
    .m  = {
        1
    },
    .entries = {
        "sdcard"
    }
};

const struct mountpoints *__get_mountpoints()
{
    return &mpoints.m;
}

int file_size(const char *path, const char *filename)
{
    char p[FILE_PATH_MAX];
    struct stat st;

    size_t plen = strlen(path);
    if (plen>0) {
        memcpy(p, path, plen+1);

        if (p[plen-1]!='/') {
            p[plen++]='/';
            p[plen]='\0';
        }
        strcpy(&p[plen], filename);
    } else {
        strcpy(p, filename);
    }
    if (stat(p,&st)<0)
        return -1;

    return st.st_size;
}

filetype_t file_type(const char *filename)
{
    struct stat st;
    if (__lstat(filename,&st)<0)
        return TYPE_INVALID;

    if ((st.st_mode&S_IFMT)== S_IFDIR) {
        return TYPE_DIRECTORY;
    }
    return TYPE_FILE;
}

struct dirent *__readdir(DIR*dir)
{
    struct dirent *d;
    do {
        d = readdir(dir);
        if (!d)
            return d;
    } while ( (d->d_type==DT_DIR) && (d->d_name[0]=='.'));
    return d;
}



void *readfile(const char *filename, int *size)
{
    struct stat st;
    void *data;
    int r;

    do {
        r = __lstat(filename, &st);
        if (r<0)
            break;
        // Read file into memory first.
        data = malloc(st.st_size);
        if (data==NULL) {
            ESP_LOGE(TAG, "Cannot allocate memory for file (%d bytes)", (int)st.st_size);
            r = -1;
            break;
        }
        int fh = __open(filename, O_RDONLY);
        if (fh<0) {
            ESP_LOGE(TAG, "Cannot open file");
            r=-1;
            break;
        }
        int readsize = read(fh, data, st.st_size);

        if (readsize!=st.st_size) {
            ESP_LOGE(TAG, "Cannot read file");
            r=-1;
            free(data);
            break;
        }
        close(fh);
        r = 0;
    } while (0);

    if (r!=0) {
        return NULL;
    }
    *size = st.st_size;
    return data;
}
