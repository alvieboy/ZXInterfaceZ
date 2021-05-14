/**
 \defgroup file File
 \brief File operations

 In order to support running in host mode, the usual POSIX functions should not be used for file
 access.
 A few wrapper functions are provided here to handle host and normal runtime mode.

 The following functions should be used:

 - __chdir (instead of chdir)
 - __opendir (instead of opendir)
 - __readdir (instead of readdir)
 - __open (instead of open)
 - __read (instead of read: to avoid -EINTR in linux)
 - __fopen (instead of fopen)
 - __lstat (instead of lopen)
 - __getcwd (instead of getcwd)

*/

#include <string.h>
#include "esp_log.h"
#include "defs.h"
#include <dirent.h>
#include "fileaccess.h"
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include "strlcpy.h"

#define TAG "FILEACCESS"

#define CWD_MAX 255

static char cwd[CWD_MAX];
static uint8_t init = 0;
static struct mountpoints system_mountpoints = { 0 };

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
        ESP_LOGI(TAG, "Statting %s", fpath);
        r = stat(fpath, st);
    } while (0);
#else
    r = stat(path, st);
#endif
    return r;
}

/**
 * \ingroup file
 * \brief Get the current working directory as a const value
 */
const char *__getcwd_const()
{
    init_cwd();
    return cwd;
}

/**
 * \ingroup file
 * \brief Get the current working directory.
 *
 * The directory name will be null-terminated.
 *
 * \param dest buffer where to place the directory name
 * \param maxlen maximum size of the buffer.
 * \return a pointer to dest.
 */

char *__getcwd(char *dest, int maxlen)
{
    init_cwd();
    strlcpy(dest, cwd, maxlen);
    return dest;
}

/**
 * \ingroup file
 * \brief Compute the full path for a file, i.e., prepend the current directory
 * \param name The file name
 * \param dest Buffer where to place the full path of the file
 * \param maxlen maximum size of the buffer
 * \return pointer to the filename in the buffer
 */

char *fullpath(const char *name, char *dest, int maxlen)
{
    char *d = dest;

    if (name[0]=='/') {
        strlcpy(dest, name, maxlen);
        return dest;
    }
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
        maxlen--;
    }
    strlcpy(d, name, maxlen);
    return d;
}


/**
 * \ingroup file
 * \brief Register a new mountpoint
 */
void register_mountpoint(const char *path)
{
    int current = system_mountpoints.count;
    system_mountpoints.mounts[current] = path;
    system_mountpoints.count++;
}

/**
 * \ingroup file
 * \brief Unregister a mountpoint
 */
void unregister_mountpoint(const char *path)
{
    int i;
    for (i=0;i<system_mountpoints.count;i++) {
        if (strcmp(system_mountpoints.mounts[i], path)==0) {
            i++;
            while (i<system_mountpoints.count) {
                system_mountpoints.mounts[i-1] = system_mountpoints.mounts[i];
            }
            system_mountpoints.count--;
            return;
        }
    }
}

/**
 * \ingroup file
 * \brief Return a list of system mountpoints
 */
const struct mountpoints *__get_mountpoints()
{
    return &system_mountpoints;
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

/**
 * \ingroup file
 * \brief Read a directory contents, skipping all entries which start with a dot.
 */
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

/**
 * \ingroup file
 * \brief Read the full contents of a file into a newly allocated memory area
 * \param filename The file to be read
 * \param size pointer to size information, which will be filled with the file size
 * \return The newly allocated area with the file contents
 */

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

int __read(int fd, void *buf, size_t len)
{
#ifndef __linux__
    return read(fd,buf,len);
#else
    int r;
    do {
        r = read(fd,buf,len);
        if (r<0) {
            if (errno!=EINTR)
                break;
        } else {
            break;
        }
    } while (1);
    return r;
#endif
}
