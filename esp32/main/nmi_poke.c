#include "nmi_poke.h"
#include "esp_log.h"
#include "sna_defs.h"
#include "fpga.h"
#include "rom.h"

#define TAG "NMIPOKE"

#define STORE(x) npoke->pokebuf[npoke->pokeidx++] = x

void nmi_poke__init(nmi_handler_poke_t *npoke)
{
    npoke->pokeidx = 0;
}

static bool nmi_poke__is_saved_address(uint8_t bank, uint16_t addr)
{
    // Addresses 0x4000 -> 0x5FFF are saved inside NMI save buffer.
    if (bank & (1<<3)) {
        if (addr>=0x4000 && addr<=0x5FFF) {
            return true;
        }
    }
    return false;
}

int nmi_poke__mem_write_fun(void *user, uint8_t bank, uint16_t address, uint8_t value)
{
    nmi_handler_poke_t *npoke = (nmi_handler_poke_t*)user;
    if (!(bank & (1<<3))) {
        ESP_LOGE(TAG,"Bank pokes still unsupported");
        return -1;
    }

    if (nmi_poke__is_saved_address(bank, address))
    {
        unsigned target_address = (address - SNA_RAM_OFF_CHUNK1_ADDR) + SNA_RAM_OFF_CHUNK1;
        ESP_LOGI(TAG,"Internal poke (z80 address %04x) -> %08x = 0x%02x", address, target_address, value);
        return fpga__write_extram(target_address, value);
    }

    ESP_LOGI(TAG,"External poke (z80 address %04x) = 0x%02x", address, value);

    STORE(0x21);
    STORE(address & 0xFF);
    STORE((address>>8) & 0xFF);     // LD HL, xxxx
    STORE(0x36);
    STORE(value & 0xFF);            // LD (HL), xx
    return 0;
}

int nmi_poke__finish(nmi_handler_poke_t *npoke)
{
    STORE(0xC9);  // RET
    return rom__load_custom_routine(npoke->pokebuf, npoke->pokeidx);
}

