#ifndef __ALIGN_H__
#define __ALIGN_H__

#define ALIGN(x, blocksize) ((((uint32_t)(x)+(blocksize-1)) & ~(blocksize-1)))

#endif
