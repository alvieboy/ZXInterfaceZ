#ifndef __BITMAP_ALLOCATOR_H__
#define __BITMAP_ALLOCATOR_H__

#include "os/semaphore.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t bitmap;
    Semaphore sem;
} bitmap32_t;

void bitmap_allocator__init(bitmap32_t *bitmap);
int bitmap_allocator__alloc(bitmap32_t *bitmap);
void bitmap_allocator__release(bitmap32_t *bitmap, int index);

#ifdef __cplusplus
}
#endif

#endif
