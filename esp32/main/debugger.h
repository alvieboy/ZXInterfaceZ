#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include <inttypes.h>
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef uint8_t z80_reg8;

#ifndef __LITTLE_ENDIAN
#error "Non little-endian systems not supported"
#endif

#define REG16(val16, val8_h, val8_l) \
    union { \
    struct { \
    uint8_t val8_l;  \
    uint8_t val8_h;  \
    }; \
    uint16_t val16; \
    }

#define REG8(val8) z80_reg8 val8

struct nmi_cpu_context_extram
{
    union {
        struct {
            REG16(BC,B,C);
            REG16(DE,D,E);
            REG16(HL,H,L);
            REG8(YM_mix);
            REG16(PC,PCh,PCl);
        } __attribute__((packed));
        uint8_t block1[9];
    };

    union {
        struct {
            REG16(SP,S,P);
            REG16(IX, IXH, IXL);
            REG16(IY, IYH, IYL);
            REG16(AF, A, F);
            REG8(R);
            REG8(unused);
            REG8(I);
            REG8(border);
            REG16(AF_alt, A_alt, F_alt);
            REG16(BC_alt, B_alt, C_alt);
            REG16(DE_alt, D_alt, E_alt);
            REG16(HL_alt, H_alt, L_alt);
        } __attribute__((packed));
        uint8_t block2[20];
    };
} __attribute__((packed));

int debugger__load_context_from_extram(struct nmi_cpu_context_extram *target);

void debugger__dump();
void debugger__dumpregs(const struct nmi_cpu_context_extram *);

#ifdef __cplusplus
}
#endif

#endif
