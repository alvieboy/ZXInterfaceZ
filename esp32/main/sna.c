#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "errno.h"
#include "defs.h"
#include "command.h"
#include "strtoint.h"
#include "sna_relocs.h"
#include "fpga.h"
#include "sna.h"

#undef TEST_ROM

extern unsigned char snaloader_rom[];
extern unsigned int snaloader_rom_len;

#ifdef TEST_ROM
extern unsigned char INTZ_ROM[];
extern unsigned int INTZ_ROM_len;
#endif


static void sna__prepare();
static int sna__chunk(command_t *cmdt);

static bool header_seen = false;

int sna__uploadsna(command_t *cmdt, int argc, char **argv)
{
    int romsize;

    if (argc<1)
        return COMMAND_CLOSE_ERROR;

    // Extract size from params.
    if (strtoint(argv[0], &romsize)<0) {
        return COMMAND_CLOSE_ERROR;
    }
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */

    ESP_LOGI(TAG, "Starting SNA upload");

    sna__prepare();

    cmdt->romsize = romsize;
    cmdt->romoffset = 0;
    cmdt->rxdatafunc = &sna__chunk;
    cmdt->state = READDATA;

    header_seen = false;

    return COMMAND_CONTINUE; // Continue receiving data.
}

static void sna__prepare()
{
    // Reset resource FIFO
    fpga__set_trigger(FPGA_FLAG_TRIG_RESOURCEFIFO_RESET);
}

static int sna__chunk(command_t *cmdt)
{
    unsigned remain = cmdt->romsize - cmdt->romoffset;

    if (remain<cmdt->len) {
        // Too much data, complain
        ESP_LOGE(TAG, "ROM: expected max %d but got %d bytes", remain, cmdt->len);
        return COMMAND_CLOSE_ERROR;
    }

    if (header_seen == false) {

        if ( cmdt->len > SNA_HEADER_SIZE ) {
#ifndef TEST_ROM
            sna_apply_relocs(cmdt->rx_buffer, snaloader_rom);
#endif
            header_seen = true;
            // Forcibly move data back, this helps below.
            cmdt->len -= SNA_HEADER_SIZE;
            memmove( cmdt->rx_buffer, &cmdt->rx_buffer[SNA_HEADER_SIZE], cmdt->len);
#ifdef TEST_ROM
            int r = fpga__upload_rom( INTZ_ROM, INTZ_ROM_len );
#else
            int r = fpga__upload_rom( snaloader_rom, snaloader_rom_len );
#endif
            if (r<0) {
                ESP_LOGE(TAG, "Error uploading ROM");
                return COMMAND_CLOSE_ERROR;
            }

            // Update len, remove the header size
            cmdt->romsize -= SNA_HEADER_SIZE;

            fpga__reset_to_custom_rom(false); // Also enable RETN hook

        } else {
            // Need more data.
            return COMMAND_CONTINUE;
        }
    }

    if (header_seen) {

        fpga__load_resource_fifo(cmdt->rx_buffer, cmdt->len, 1000);

        cmdt->romoffset += cmdt->len;
        cmdt->len = 0; // Reset receive ptr.
        remain = cmdt->romsize - cmdt->romoffset;

        ESP_LOGI(TAG, "SNA: remain size %d", remain);

        if (remain==0) {
            return COMMAND_CLOSE_OK;
        }

        return COMMAND_CONTINUE;
    }

    return COMMAND_CONTINUE;
}


