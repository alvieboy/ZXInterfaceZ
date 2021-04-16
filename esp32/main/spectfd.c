#include "spectfd.h"
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_SPECT_FD 16
#define SPECTFD_RESERVED (0)

static int spectfd_map[MAX_SPECT_FD];

void spectfd__init(void)
{
    memset(spectfd_map, 0xFF, sizeof(spectfd_map));
}

static bool spectfd__is_valid_system(int index)
{
    return (spectfd_map[index]>SPECTFD_RESERVED);
}

static bool spectfd__is_valid_system_or_reserved(int index)
{
    return (spectfd_map[index]>SPECTFD_RESERVED);
}

void spectfd__close_all(void)
{
    unsigned i;
    for (i=0;i<MAX_SPECT_FD;i++) {
        if (!spectfd__is_valid_system(i))
            continue;
        close(spectfd_map[i]);
        spectfd_map[i] = SPECTFD_ERROR;
    }
}

int spectfd__spect_to_system(spectfd_t fd)
{
    if ((fd < 0) || (fd>=MAX_SPECT_FD))
        return -1;

    return spectfd_map[fd];
}

spectfd_t spectfd__alloc(int systemfd)
{
    unsigned i;
    for (i=0;i<MAX_SPECT_FD;i++) {
        if (spectfd__is_valid_system_or_reserved(i)) {
            continue;
        }
        spectfd_map[i] = systemfd;
        return i;
    }
    return SPECTFD_ERROR;
}

spectfd_t spectfd__prealloc()
{
    return spectfd__alloc( SPECTFD_RESERVED );
}

void spectfd__update_systemfd(spectfd_t fd, int systemfd)
{
    spectfd_map[fd] = systemfd;
}

int spectfd__close(spectfd_t fd)
{
    int sysfd = spectfd_map[fd];
    if (spectfd__is_valid_system(fd))
        close(sysfd);

    spectfd_map[fd] = SPECTFD_ERROR;
    return 0;
}


