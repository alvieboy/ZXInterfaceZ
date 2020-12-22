#include "sna_z80.h"
#include "wsys/core.h"

struct checks {

    static_assert(sizeof(sna_z80_main_header_t)==30);
    static_assert(sizeof(sna_z80_ext_header_t)==57);
    static_assert(sizeof(sna_z80_block_header_t)==3);
    static_assert(sizeof(attr_t)==1);
};


