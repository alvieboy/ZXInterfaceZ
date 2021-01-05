#include "divcompat.h"
#include "rom_hook.h"
#include "fpga.h"

static int8_t divhooks[7] = {-1};

/*

Automatic mapping occurs at the begining of refresh cycle after fetching an
opcode (after the M1 cycle) from 0000h, 0008h, 0038h, 0066h, 04C6h and 0562h.
Mapping also occurs instantly when executing an opcode at 3D00h-3DFFh, 100 ns
after the /MREQ falling edge.

DivIDE memory is paged out in the refresh cycle of any instruction fetch
from 1FF8h-1FFFh, referred to as the 'off-area'.

*/

int divcompat__enable(uint8_t rom)
{
#define DIVCHECK if (divhooks[i]<0) break; i++

    int i = 0;
    do {
        // Page in, immediate, 3D00-3DFF range.
        divhooks[i] = rom_hook__add_pre_set(rom, 0x3D00, 0);  // 256 bytes, 0 will underflow. This is intended.
        DIVCHECK;
        // Page out, off-area
        divhooks[i] = rom_hook__add_post_reset(rom, 0x1FF8, 8);
        DIVCHECK;
        // RST handlers
        divhooks[i] = rom_hook__add_post_set(rom, 0x0000, 1);
        DIVCHECK;
        divhooks[i] = rom_hook__add_post_set(rom, 0x0008, 1);
        DIVCHECK;
        divhooks[i] = rom_hook__add_post_set(rom, 0x0038, 1); // Interrupt handler
        DIVCHECK;
        //divhook [2] = rom_hook__add_post_set(rom, 0x0066, 1); // NMI handler. TBD.
        divhooks[i] = rom_hook__add_post_set(rom, 0x04C6, 1); // SA-BYTES (we think)
        DIVCHECK;
        divhooks[i] = rom_hook__add_post_set(rom, 0x0562, 1); // LD-BYTES (we think)
        DIVCHECK;

        fpga__set_config1_bits(CONFIG1_DIVMMC_COMPAT);
        return 0;
    } while (0);
    // Error occurred
    divcompat__disable();
    return -1;
}

void divcompat__disable()
{
    int i;
    for (i=0;i<sizeof(divhooks);i++) {
        if(divhooks[i]>=0) {
            rom_hook__remove(divhooks[i]);
        }
    }
}