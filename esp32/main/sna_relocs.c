#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include "sna_relocs.h"
#include "fpga.h"

typedef enum {
    RELOC_16,
    RELOC_8,
    RELOC_IM,
    RELOC_EIDI
} reloctype_t;

struct relocentry
{
    const char *name;
    uint16_t offset;
    reloctype_t type;
};

const struct relocentry relocmap[] = {
    { "AF'", 0x0005, RELOC_16 },

    { "BC'", 0x000F, RELOC_16 },
    { "DE'", 0x0012, RELOC_16 },
    { "HL'", 0x0015, RELOC_16 },
    { "I",   0x001B, RELOC_8 },
    { "AF",  0x001F, RELOC_16 },
    { "BC",  0x0026, RELOC_16 },
    { "R",   0x002C, RELOC_8 },
    { "BORDER",   0x0030, RELOC_8 },
    { "DE",   0x0035, RELOC_16 },
    { "IX",   0x0039, RELOC_16 },
    { "IY",   0x003D, RELOC_16 },
    { "SP",   0x0043, RELOC_16 },
    { "HL",   0x0046, RELOC_16 },
    { "IM",   0x0049, RELOC_IM },  // 0x46, 0x56, 0x5E,
    { "INT",  0x004A, RELOC_EIDI},  // EI: 0xFB, DI: 0xF3
};

static const struct relocentry *findreloc(const char *name)
{
    unsigned i;
    const struct relocentry *e;

    for(i=0;i<sizeof(relocmap)/sizeof(relocmap[0]); i++) {
        e = &relocmap[i];
        if (strcmp(name,e->name)==0) {
            return e;
        }
    }
    return NULL;
}

// Offset shall be 0x0080 for the "old" method, and 0x1F00 for the in-main-rom method

static int apply_single_reloc(const char *name, const uint8_t *src, sna_writefun fun, void *userdata, uint16_t offset)
{
    const struct relocentry *e = findreloc(name);
    uint8_t v;

    if (!e)
        return -1;
    switch (e->type) {
    case RELOC_16:
        fun(userdata, offset + e->offset, src[0]);
        fun(userdata, offset + e->offset+1, src[1]);
        break;
    case RELOC_8:
        fun(userdata, offset + e->offset, src[0]);
        break;
    case RELOC_IM:
        switch (src[0]) {
        case 0: v = 0x46; break;
        case 1: v = 0x56; break;
        case 2: v = 0x5E; break;
        default:
            fprintf(stderr,"Unknown IM mode %d\n", src[0]);
            return -1;
        }
        fun(userdata, offset + e->offset, v);
        break;
    case RELOC_EIDI:
        if (src[0]&4) {
            // EI
            v = 0xFB;
        }  else {
            v = 0xF3;
        }
        fun(userdata, offset + e->offset, v);
        break;
    }
    return 0;
}

static void writefun_mem(void *data, unsigned offset, uint8_t val)
{
    uint8_t *p = (uint8_t*)data;
    p[ offset ] = val;
}

static void writefun_extram(void *data, unsigned offset, uint8_t val)
{
    unsigned p = *((unsigned*)data) + offset;
    fpga__write_extram(p, val);
}

static void writefun_fpgarom(void *data, unsigned offset, uint8_t val)
{
    fpga__write_rom(offset, val);
}

void sna_apply_relocs_mem(const uint8_t *sna, uint8_t *rom, uint16_t offset)
{
    sna_apply_relocs(sna, offset, &writefun_mem, rom);
}

static void sna_apply_relocs_extram(const uint8_t *sna, unsigned extram_address, uint16_t offset)
{
    sna_apply_relocs(sna, offset, &writefun_extram, (void*)&extram_address);
}

void sna_apply_relocs_fpgarom(const uint8_t *sna, uint16_t offset)
{
    sna_apply_relocs(sna, offset, &writefun_fpgarom, (void*)NULL);
}

void sna_apply_relocs(const uint8_t *sna, uint16_t offset, sna_writefun fun, void *userdata)
{
    apply_single_reloc("I", &sna[0], fun, userdata, offset);
    apply_single_reloc("HL'", &sna[1], fun, userdata, offset);
    apply_single_reloc("DE'", &sna[3], fun, userdata, offset);
    apply_single_reloc("BC'", &sna[5], fun, userdata, offset);
    apply_single_reloc("AF'", &sna[7], fun, userdata, offset);
    apply_single_reloc("HL", &sna[9], fun, userdata, offset);
    apply_single_reloc("DE", &sna[11], fun, userdata, offset);
    apply_single_reloc("BC", &sna[13], fun, userdata, offset);
    apply_single_reloc("IY", &sna[15], fun, userdata, offset);
    apply_single_reloc("IX", &sna[17], fun, userdata, offset);
    apply_single_reloc("INT", &sna[19], fun, userdata, offset);
    apply_single_reloc("R", &sna[20], fun, userdata, offset);
    apply_single_reloc("AF", &sna[21], fun, userdata, offset);
    apply_single_reloc("SP", &sna[23], fun, userdata, offset);
    apply_single_reloc("IM", &sna[25], fun, userdata, offset);
    apply_single_reloc("BORDER", &sna[26], fun, userdata, offset);

/*
;   ------------------------------------------------------------------------
;   0        1      byte   I                                      Check
;   1        8      word   HL',DE',BC',AF'                        Check
;   9        10     word   HL,DE,BC,IY,IX                         Check
;   19       1      byte   Interrupt (bit 2 contains IFF2, 1=EI/0=DI)  Check
;   20       1      byte   R                                      Check
;   21       4      words  AF,SP                                  Check
;   25       1      byte   IntMode (0=IM0/1=IM1/2=IM2)            Check
;   26       1      byte   BorderColor (0..7, not used by Spectrum 1.7)  Check
;   27       49152  bytes  RAM dump 16384..65535
;   ------------------------------------------------------------------------
*/

}
