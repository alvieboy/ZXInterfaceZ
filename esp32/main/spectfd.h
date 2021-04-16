#ifndef __SPECTFD_H__
#define __SPECTFD_H__

#include <inttypes.h>
/**
 * \defgrou Spectrum file descriptors
 * \ingroup misc
 */

typedef int8_t spectfd_t;

#define SPECTFD_ERROR (-1)

void spectfd__init(void);
void spectfd__close_all(void);
int spectfd__spect_to_system(spectfd_t fd);
spectfd_t spectfd__alloc(int systemfd);
spectfd_t spectfd__prealloc();
void spectfd__update_systemfd(spectfd_t fd, int systemfd);
int spectfd__close(spectfd_t fd);

#endif
