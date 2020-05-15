#ifndef __VIDEOSTREAMER_H__
#define __VIDEOSTREAMER_H__

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

void videostreamer__init(void);
int videostreamer__start_stream(struct in_addr addr, uint16_t port);
const uint8_t *videostreamer__getlastfb();
unsigned videostreamer__getinterrupts(void);

#endif
