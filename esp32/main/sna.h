#ifndef __SNA_H__
#define __SNA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "command.h"

#define ROM_PATCHED_SNALOAD_ADDRESS 0x1F00

#define SNA_EXTRAM_ADDRESS 0x010000
#define SNA_Z80_EXTRAM_BASE_PAGE_ADDRESS 0x030000
#define SNA_Z80_EXTRAM_PAGE_ALLOCATION_ADDRESS 0x01C010
#define SNA_Z80_EXTRAM_LAST7FFD_ADDRESS 0x01C020
#define SNA_Z80_EXTRAM_YM_REGISTERCONTENT_ADDRESS 0x01C010
#define SNA_Z80_EXTRAM_YM_LASTREGISTER_ADDRESS 0x01C01A


/*
 EXTERNAL MEMORY LAYOUT for extram loads.

 SNA
 +-----------+--------+------------------------------------------+
 | Address   | Size   | Contents                                 |
 +-----------+--------+------------------------------------------+
 | 0x010000  | 0xC000 | RAM contents 0x4000-0xFFFF               |
 +-----------+--------+------------------------------------------+

 Z80v1/v2/v3/v3+
 +-----------+--------+------------------------------------------+
 | Address   | Size   | Contents                                 |
 +-----------+--------+------------------------------------------+
 | 0x01C000  | 0x10   | YM registers (16)                        |
 | 0x01C010  | 0x08   | Page allocations                         |
 | 0x01C019  | 0x01   | 0xFF                                     |
 | 0x01C01A  | 0x01   | YM current register                      |
 | 0x01C020  | 0x01   | Last out to 0x7FFd                       |
 | 0x030000  | 0x4000 | Page A                                   |
 | 0x034000  | 0x4000 | Page B..                                 |
 | ...       | 0x4000 | Page n..                                 |
 | 0x04C000  | 0x4000 | Page H                                   |
 +-----------+--------+------------------------------------------+

 If page allocation is 0xFF, page is not present and no more pages
 exist.

 */

typedef enum {
    SNAPSHOT_SNA = 0,
    SNAPSHOT_Z80_V1,
    SNAPSHOT_Z80_V2,
    SNAPSHOT_Z80_V3,
    SNAPSHOT_Z80_V3PLUS,
    SNAPSHOT_ERROR=-1
} snatype_t;

int sna__uploadsna(command_t *cmdt, int argc, char **argv);
int sna__save_from_extram(const char *name);
const char *sna__get_error_string(void);
snatype_t sna__load_snapshot_extram(const char *file);
//int sna__load_z80_extram(const char *file);


#ifdef __cplusplus
}
#endif

#endif
