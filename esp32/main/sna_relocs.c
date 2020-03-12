#include <inttypes.h>
#include <string.h>
#include <stdio.h>

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
    { "AF'", 0x0086, RELOC_16 },
    { "BC'", 0x0090, RELOC_16 },
    { "DE'", 0x0093, RELOC_16 },
    { "HL'", 0x0096, RELOC_16 },
    { "I",   0x009B, RELOC_8 },
    { "AF",  0x009F, RELOC_16 },
    { "BC",  0x00A6, RELOC_16 },
    { "R",   0x00AC, RELOC_8 },
    { "BORDER",   0x00B0, RELOC_8 },
    { "DE",   0x00B5, RELOC_16 },
    { "IX",   0x00B9, RELOC_16 },
    { "IY",   0x00BD, RELOC_16 },
    { "SP",   0x00C3, RELOC_16 },
    { "HL",   0x00C6, RELOC_16 },
    { "IM",   0x00C9, RELOC_IM },  // 0x46, 0x56, 0x5E,
    { "INT",  0x00CA, RELOC_EIDI},  // EI: 0xFB, DI: 0xF3
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

static int apply_single_reloc(const char *name, const uint8_t *src, uint8_t *rom)
{
    const struct relocentry *e = findreloc(name);
    uint8_t v;

    if (!e)
        return -1;
    switch (e->type) {
    case RELOC_16:
        rom[ e->offset ] = src[0];
        rom[ e->offset+1 ] = src[1];
        break;
    case RELOC_8:
        rom[ e->offset ] = src[0];
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
        rom[ e->offset ] = v;
        break;
    case RELOC_EIDI:
        if (src[0]&1) {
            // EI
            v = 0xFB;
        }  else {
            v = 0xF3;
        }
        rom[ e->offset ] = v;
        break;
    }
    return 0;
}



void sna_apply_relocs(const uint8_t *sna, uint8_t *rom)
{
    apply_single_reloc("I", &sna[0], rom);
    apply_single_reloc("HL'", &sna[1], rom);
    apply_single_reloc("DE'", &sna[3], rom);
    apply_single_reloc("BC'", &sna[5], rom);
    apply_single_reloc("AF'", &sna[7], rom);
    apply_single_reloc("HL", &sna[9], rom);
    apply_single_reloc("DE", &sna[11], rom);
    apply_single_reloc("BC", &sna[13], rom);
    apply_single_reloc("IY", &sna[15], rom);
    apply_single_reloc("IX", &sna[17], rom);
    apply_single_reloc("INT", &sna[19], rom);
    apply_single_reloc("R", &sna[20], rom);
    apply_single_reloc("AF", &sna[21], rom);
    apply_single_reloc("SP", &sna[23], rom);
    apply_single_reloc("IM", &sna[25], rom);
    apply_single_reloc("BORDER", &sna[26], rom);

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
