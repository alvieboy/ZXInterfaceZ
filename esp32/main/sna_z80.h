#ifndef __SNA_Z80_H__
#define __SNA_Z80_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t a;
    uint8_t f;
    uint16_t bc;
    uint16_t hl;
    uint16_t pc;
    uint16_t sp;
    uint8_t i;
    uint8_t r;
    union {
        struct {
            uint8_t r_bit7:1;
            uint8_t border:3;
            uint8_t samrom:1;
            uint8_t compressed:1;
            uint8_t pad:2;
        } __attribute((packed));
        uint8_t flags;
    };
    uint16_t de;
    uint16_t bc_alt;
    uint16_t de_alt;
    uint16_t hl_alt;
    uint8_t a_alt;
    uint8_t f_alt;
    uint16_t iy;
    uint16_t ix;
    uint8_t iff1;
    uint8_t iff2;
    union {
        struct {
            uint8_t im:2;
            uint8_t issue2_emul:1;
            uint8_t double_intf:1;
            uint8_t videosync: 2;
            uint8_t josticks:2;
        };
      uint8_t flags2;
    };
} __attribute__((packed)) sna_z80_main_header_t ;


typedef struct {
    uint16_t len;
    uint16_t pc;
    uint8_t hwmode;
    uint8_t out1;
    uint8_t paged;
    union {
        struct {
            uint8_t r_emul:1;
            uint8_t ldir_emul:1;
            uint8_t ay_sound:1;
            uint8_t pad:3;
            uint8_t fuller_emul:1;
            uint8_t modify_hw:1;
        };
        uint8_t flags;
    };
    uint8_t soundchipregno;
    uint8_t soundchipregs[16];
    uint16_t lowtstate;
    uint8_t hightstate;
    uint8_t spectator_flag;
    uint8_t mgtrom_paged;
    uint8_t multiface_paged;
    uint8_t block0rom; // First 8K
    uint8_t block1rom; // Second 8K
    uint8_t userdefjoykey[10];
    uint8_t userdefjoyascii[10];
    uint8_t mgt;
    uint8_t inhibitbutton;
    uint8_t inhibitstat;
    uint8_t lastout;
} __attribute__((packed)) sna_z80_ext_header_t;

typedef struct {
    uint16_t len;
    uint8_t block;
} __attribute__((packed)) sna_z80_block_header_t;

typedef struct z80_decomp {
    uint8_t version;
    enum {
        Z80D_RAW,
        Z80D_ED0,
        Z80D_ED1,
        Z80D_LEN,
        // For EOF on V1.
        Z80D_ZERO,
        Z80D_ZED0,
        Z80D_ZED1,
        Z80D_EOF
    } state;
    uint8_t repcnt;
    uint16_t bytes_output;
    int (*writer)(void *arg, uint8_t val);
    void *writer_arg;
} z80_decomp_t;

#ifdef __cplusplus
}
#endif

#endif
