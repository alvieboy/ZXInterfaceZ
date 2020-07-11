#ifndef __FLASHPGM_H__
#define __FLASHPGM_H__

#include "esp_partition.h"

#define FLASH_BLOCK_SIZE 4096

#define ALIGN(x, blocksize) ((((uint32_t)(x)+(blocksize-1)) & ~(blocksize-1)))

typedef struct
{
    const esp_partition_t *partition;
    unsigned offset;
} flash_program_handle_t;

int flash_pgm__prepare_programming(flash_program_handle_t*handle,
                                  esp_partition_type_t type,
                                  esp_partition_subtype_t subtype,
                                  const char *label,
                                  unsigned len);
int flash_pgm__program_chunk(flash_program_handle_t *handle, const uint8_t *data, unsigned len);



#endif
