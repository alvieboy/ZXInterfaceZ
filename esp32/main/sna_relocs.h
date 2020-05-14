#ifndef __SNA_RELOCS_H__
#define __SNA_RELOCS_H__

#include <inttypes.h>

#define SNA_HEADER_SIZE 27

typedef void (*sna_writefun)(void *data, unsigned offset, uint8_t val);

void sna_apply_relocs_fpgarom(const uint8_t *sna, uint16_t offset);
void sna_apply_relocs_mem(const uint8_t *sna, uint8_t *rom, uint16_t offset);
void sna_apply_relocs(const uint8_t *sna, uint16_t offset, sna_writefun fun, void *userdata);

#endif
