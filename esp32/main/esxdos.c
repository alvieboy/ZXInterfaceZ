#include "esxdos.h"
#include "resource.h"
#include <string.h>
#include "esp_log.h"
#include <stdio.h>
#include "fileaccess.h"
#include "memlayout.h"
#include "fpga.h"
#include <unistd.h>

#define ESXDOSTAG "ESXDOS"

// We register 2 disks. First is spiffs, second is sdcard.
struct esxdos_disk_info {
    uint8_t id; // LD	(IX),  $08  ; IDE
    uint8_t flags;
    uint32_t size;
} __attribute__((packed));

struct esxdos_disk_info disks[] = {
    { .id=0x08, .flags = 0xFF, .size = 0x10000 },
//    { .id=0x88, .flags = 0xFF, .size = 0x10000 },
};

const char *drive_mountpoints[] = {
    "/spiffs",
    "/sdcard"
};

static uint8_t current_drive = 0;

// File handles.

#define MAX_SPECTRUM_FILE_HANDLES 4
static int spectrum_fh[MAX_SPECTRUM_FILE_HANDLES] = { -1 };

static int esxdos__get_free_handle()
{
    int i;
    for (i=0;i<MAX_SPECTRUM_FILE_HANDLES;i++) {
        if (spectrum_fh[i]==-1)
            return i;
    }
    return -1;
}


int esxdos__diskinfo(uint8_t drive)
{
    uint8_t buffer[16];
    unsigned len;

    ESP_LOGI(ESXDOSTAG,"Request DISK_INFO for 0x%02x", drive);

    if (drive==0) {
        len = sizeof(disks);
        memcpy(buffer, disks, len);
        buffer[len++] = '\0';
    } else {
        // Locate disk. TBD
        len = 0;
    }

    if (len>0) {
        return resource__sendrawbuffer(0x20,buffer,len);
    }
    return 0;
}

int esxdos__driveinfo(int8_t drive)
{
    uint8_t info[32];
    int pos = 0;

    ESP_LOGI(ESXDOSTAG,"Request DRIVE_INFO for drive 0x%02x", drive);
    info[pos++] = 0x40;
    info[pos++] = 0x09;
    info[pos++] = 0xFF;
    info[pos++] = 0xFF;
    info[pos++] = 0x00;
    info[pos++] = 0x00;
    info[pos++] = 0x10;
    info[pos++] = 0x00;
    // Filesystem
    info[pos++] = 'F';
    info[pos++] = 'A';
    info[pos++] = 'T';
    info[pos++] = 0x00;
    // Label
    info[pos++] = 'E';
    info[pos++] = 'S';
    info[pos++] = 'X';
    info[pos++] = 0x00;

    return resource__sendrawbuffer(0x21,info, pos);
}

// Try to open file. TBD: file mode
static int esxdos__open_filesystem_file(const char *name, uint8_t mode)
{
    int realmode = 0;
    char fullpath[128];
    sprintf(fullpath, "%s/%s", drive_mountpoints[current_drive], name);

    if ((mode & (ESXDOS_OPEN_FILEMODE_READ|ESXDOS_OPEN_FILEMODE_WRITE)) == (ESXDOS_OPEN_FILEMODE_READ|ESXDOS_OPEN_FILEMODE_WRITE)) {
        realmode |= O_RDWR;
    } else {
        if (mode&ESXDOS_OPEN_FILEMODE_READ)
            realmode |= O_RDONLY;
        if (mode&ESXDOS_OPEN_FILEMODE_WRITE)
            realmode |= O_WRONLY;
    }
    if (mode & ESXDOS_OPEN_FILEMODE_CREAT_NOEXIST)
        realmode |= O_CREAT;
    if (mode & ESXDOS_OPEN_FILEMODE_CREAT_TRUNC)
        realmode |= O_TRUNC;

    return __open(fullpath, realmode, 0666);
}


int esxdos__open(const char *name, uint8_t mode)
{
    int r = -1;

    ESP_LOGI(ESXDOSTAG,"Request OPEN for file '%s', mode 0x%02x", name, mode);

    int8_t handle = esxdos__get_free_handle();

    if (handle>=0) {
        int realhandle = esxdos__open_filesystem_file(name, mode);
        if (realhandle>=0) {
            spectrum_fh[ handle ] = realhandle;
            r = 0;
        }
    }

    if (r<0) {
        return resource__sendrawbuffer(0xFF, NULL, 0);
    } else {
        return resource__sendrawbuffer(0x22, (uint8_t*)&handle, 1);
    }
}

int esxdos__close(uint8_t handle)
{
    int r = -1;

    if (handle<MAX_SPECTRUM_FILE_HANDLES) {
        int realhandle = spectrum_fh[handle];
        if (realhandle>=0) {
            r = close(realhandle);
            spectrum_fh[handle] = -1;
        }
    }
    if (r<0) {
        return resource__sendrawbuffer(0xFF, NULL, 0);
    } else {
        return resource__sendrawbuffer(0x00, NULL, 0);
    }
}

int esxdos__read(uint8_t handle, uint16_t len)
{
    ESP_LOGI(ESXDOSTAG,"Request READ for fh 0x%02x len=%d", handle, len);
    int written;

    int r = -1;
    if (handle<MAX_SPECTRUM_FILE_HANDLES) {
        int realhandle = spectrum_fh[handle];
        if (realhandle>=0) {
            // read data
            r = fpga__write_extram_block_from_file_nonblock(MEMLAYOUT_ESXDOS_FILEBUFFER,
                                                            realhandle,
                                                            len,
                                                            &written);
        }
    }

    if (r<0) {
        ESP_LOGE(ESXDOSTAG,"Short read");
        return resource__sendrawbuffer(0xFF, NULL, 0);
    } else {
        ESP_LOGI(ESXDOSTAG,"Read successful %d bytes", written);
        uint8_t buf[5];
        buf[0] = written;
        buf[1] = written>>8;     // Read len.
        buf[2] = (MEMLAYOUT_ESXDOS_FILEBUFFER >> 16) & 0xff;
        buf[3] = (MEMLAYOUT_ESXDOS_FILEBUFFER >> 8) & 0xff;
        buf[4] = (MEMLAYOUT_ESXDOS_FILEBUFFER >> 0) & 0xff;
        return resource__sendrawbuffer(0x23, buf, 5);
    }
}

