#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include "sna_relocs.h"
#include "fpga.h"
#include "sna_z80.h"
#include "esp_log.h"

typedef enum {
    RELOC_16,
    RELOC_8,
    RELOC_IM,
    RELOC_EIDI,
    RELOC_PUSHPC
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
    { "HL",   0x004A, RELOC_16 },
    { "IM",   0x004D, RELOC_IM },  // 0x46, 0x56, 0x5E,
    { "INT",  0x004E, RELOC_EIDI},  // EI: 0xFB, DI: 0xF3,
//    { "EXC",  0x004B, RELOC_EXECINSN}, // Exec instruction (RETN or JP)
    { "PC",   0x0045, RELOC_PUSHPC }, // Jump PC for JP in EXC
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

static int apply_single_reloc(const char *name, const void *srcv, sna_writefun fun, void *userdata, uint16_t offset)
{
    const struct relocentry *e = findreloc(name);
    uint8_t v;
    const uint8_t *src = (const uint8_t *)srcv;

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
    case RELOC_PUSHPC:
        if (src[0]==0 && src[1]==0) {
            fun(userdata, offset + e->offset, 0x00);  // NOP
            fun(userdata, offset + e->offset+1, 0x00);  // NOP
            fun(userdata, offset + e->offset+2, 0x00);  // NOP
            fun(userdata, offset + e->offset+3, 0x00);  // NOP
        } else {
            fun(userdata, offset + e->offset, 0x21);  // LD HL, xxxx
            fun(userdata, offset + e->offset+1, src[0]);  // LD HL, xxxx
            fun(userdata, offset + e->offset+2, src[1]);  // LD HL, xxxx
            fun(userdata, offset + e->offset+3, 0xE5);  // PUSH HL
        }
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

void sna_z80_apply_relocs_fpgarom(const sna_z80_main_header_t *main,
                                  const sna_z80_ext_header_t *ext,
                                  uint16_t offset)
{
    sna_z80_apply_relocs(main, ext, offset, &writefun_fpgarom, (void*)NULL);
}


void sna_apply_relocs(const uint8_t *sna, uint16_t offset, sna_writefun fun, void *userdata)
{
    uint16_t zero  = 0;
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
    apply_single_reloc("PC", &zero, fun, userdata, offset);
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

void sna_z80_apply_relocs(const sna_z80_main_header_t *main,
                      const sna_z80_ext_header_t *ext,
                      uint16_t offset,
                      sna_writefun fun,
                      void *userdata)
{
    uint8_t imval = main->im;
    uint8_t border = main->border;

    apply_single_reloc("I", &main->i, fun, userdata, offset);
    apply_single_reloc("HL'", &main->hl_alt, fun, userdata, offset);
    apply_single_reloc("DE'", &main->de_alt, fun, userdata, offset);
    apply_single_reloc("BC'", &main->bc_alt, fun, userdata, offset);
    apply_single_reloc("AF'", &main->a_alt, fun, userdata, offset); // CHECK AF!
    apply_single_reloc("HL", &main->hl, fun, userdata, offset);
    apply_single_reloc("DE", &main->de, fun, userdata, offset);
    apply_single_reloc("BC", &main->bc, fun, userdata, offset);
    apply_single_reloc("IY", &main->iy, fun, userdata, offset);
    apply_single_reloc("IX", &main->ix, fun, userdata, offset);
    apply_single_reloc("INT", &main->iff1, fun, userdata, offset);
    apply_single_reloc("R", &main->r, fun, userdata, offset);
    apply_single_reloc("AF", &main->a, fun, userdata, offset); // Check AF!
    apply_single_reloc("SP", &main->sp, fun, userdata, offset);
    apply_single_reloc("IM", &imval, fun, userdata, offset);
    apply_single_reloc("BORDER", &border, fun, userdata, offset);

    if (main->pc!=0) {
        ESP_LOGI("Z80","Using PC %04x", main->pc);
        apply_single_reloc("PC", &main->pc, fun, userdata, offset);
    } else {
        ESP_LOGI("Z80","Using PC (ext) %04x", ext->pc);
        apply_single_reloc("PC", &ext->pc, fun, userdata, offset);
    }

    // Dump ROM
#ifdef __linux__
    {
        uint8_t buf[256];
        int f = open("Z80.bin", O_WRONLY|O_CREAT|O_TRUNC, 0667);
        fpga__read_extram_block(offset, buf, sizeof(buf));
        write(f, buf, sizeof(buf));
        close(f);
    }
#endif

}
