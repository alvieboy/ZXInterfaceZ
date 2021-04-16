#ifndef __NETWORKAPI_H__
#define __NETWORKAPI_H__

#include "spectfd.h"

/**
 * \defgroup networkapi Network API
 * \brief ZX Spectrum network API
 */

int networkapi__socket(uint8_t type);
int networkapi__gethostbyname(const char *name, uint32_t *target);
int networkapi__connect(spectfd_t fd, uint32_t host, uint16_t port);
int networkapi__sendto(spectfd_t fd, uint32_t host, uint16_t port, const uint8_t *data, uint16_t datalen);



#endif
