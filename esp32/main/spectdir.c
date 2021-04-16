#include "spectdir.h"
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_SPECT_DIR 4

static DIR* spectdir_map[MAX_SPECT_DIR];
#define SPECTDIR_RESERVED ((DIR*)(0xFFFFFFFF))

void spectdir__init(void)
{
    memset(spectdir_map, 0x00, sizeof(spectdir_map));
}

static bool spectdir__is_valid_system(int index)
{
    return (spectdir_map[index]!=NULL && spectdir_map[index]!=SPECTDIR_RESERVED);
}

static bool spectdir__is_valid_system_or_reserved(int index)
{
    return spectdir_map[index]!= NULL;
}

void spectdir__close_all(void)
{
    unsigned i;
    for (i=0;i<MAX_SPECT_DIR;i++) {
        if (!spectdir__is_valid_system(i))
            continue;
        closedir(spectdir_map[i]);
        spectdir_map[i] = NULL;
    }
}

DIR *spectdir__spect_to_system(spectdirhandle_t handle)
{
    if ((handle < 0) || (handle>=MAX_SPECT_DIR))
        return NULL;

    return spectdir_map[handle];
}

spectdirhandle_t spectdir__alloc(DIR*d)
{
    unsigned i;
    for (i=0;i<MAX_SPECT_DIR;i++) {
        if (spectdir__is_valid_system_or_reserved(i)) {
            continue;
        }
        spectdir_map[i] = d;
        return i;
    }
    return SPECTDIR_ERROR;
}

spectdirhandle_t spectdir__prealloc()
{
    return spectdir__alloc( SPECTDIR_RESERVED );
}

void spectdir__update_dirhandle(spectdirhandle_t h, DIR *handle)
{
    spectdir_map[h] = handle;
}

int spectdir__close(spectdirhandle_t fd)
{
    DIR *handle = spectdir_map[fd];
    if (spectdir__is_valid_system(fd))
        closedir(handle);
    spectdir_map[fd] = NULL;
    return 0;
}


