#ifndef __RLE_H__
#define __RLE_H__

#include <inttypes.h>
#include <sys/types.h>

#define MIN_RUN     5                   /* minimum run length to encode */
#define MAX_RUN     (32767 + MIN_RUN - 1) /* maximum run length to encode */
#define MAX_COPY    32767                 /* maximum characters to copy */
#define MAX_READ    (MAX_COPY + MIN_RUN - 1)

int rle_decompress_stream(int (*reader)(void*user, uint8_t*buf,size_t), void *read_user,
                          int (*writer)(void*user, const uint8_t*buf,size_t), void *write_user,
                          int sourcelen);


#endif
