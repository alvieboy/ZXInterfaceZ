#ifndef __ALIGN_H__
#define __ALIGN_H__

/**
 * \ingroup misc
 * \brief Align a number to a certain block size
 */
#define ALIGN(x, blocksize) ((((uint32_t)(x)+(blocksize-1)) & ~(blocksize-1)))

#endif
