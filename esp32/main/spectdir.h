#ifndef __SPECTDIR_H__
#define __SPECTDIR_H__

#include <inttypes.h>
#include <dirent.h>
/**
 * \defgrou Spectrum directory descriptors
 * \ingroup misc
 */

typedef int8_t spectdirhandle_t;

#define SPECTDIR_ERROR (-1)

void spectdir__init(void);
void spectdir__close_all(void);
spectdirhandle_t spectdir__alloc_dirhandle(DIR *);
spectdirhandle_t spectdir__prealloc_dirhandle();
void spectdir_update_dirhandle(spectdirhandle_t, DIR*);
int spectdir__close_dirhandle(spectdirhandle_t);
DIR* spectdir__spect_to_system(spectdirhandle_t fd);

#endif
