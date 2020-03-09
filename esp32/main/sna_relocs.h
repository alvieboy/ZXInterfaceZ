#ifndef __SNA_RELOCS_H__
#define __SNA_RELOCS_H__

#include <inttypes.h>

#define SNA_HEADER_SIZE 27

void sna_apply_relocs(const uint8_t *sna, uint8_t *rom);

#endif
