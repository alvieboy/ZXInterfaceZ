#include "spectfd.h"
#include <string.h>
#include <unistd.h>

#define MAX_SPECT_FD 16

static int spectfd_map[MAX_SPECT_FD];

void spectfd__init(void)
{
    memset(spectfd_map, 0xFF, sizeof(spectfd_map));
}

void spectfd__close_all(void)
{
    unsigned i;
    for (i=0;i<MAX_SPECT_FD;i++) {
        if (spectfd_map[i]<0)
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
        if (spectfd_map[i]<0) {
            continue;
        }
        spectfd_map[i] =  systemfd;
        return i;
    }
    return SPECTFD_ERROR;
}
