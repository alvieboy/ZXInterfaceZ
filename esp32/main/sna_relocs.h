#ifndef __SNA_RELOCS_H__
#define __SNA_RELOCS_H__

#include <inttypes.h>
#include "sna_z80.h"

#define SNA_HEADER_SIZE 27

typedef void (*sna_writefun)(void *data, unsigned offset, uint8_t val);

void sna_apply_relocs_fpgarom(const uint8_t *sna, uint16_t offset);
void sna_apply_relocs_mem(const uint8_t *sna, uint8_t *rom, uint16_t offset);
void sna_apply_relocs(const uint8_t *sna, uint16_t offset, sna_writefun fun, void *userdata);
void sna_z80_apply_relocs(const sna_z80_main_header_t *main,
                          const sna_z80_ext_header_t *ext,
                          uint16_t offset,
                          sna_writefun fun,
                          void *userdata);

void sna_z80_apply_relocs_fpgarom(const sna_z80_main_header_t *main,
                                  const sna_z80_ext_header_t *ext,
                                  uint16_t offset);

#endif
