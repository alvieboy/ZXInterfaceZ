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
#include "dump.h"
#include "sna_defs.h"
#include <stdlib.h>

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
            dump__buffer(snaloader_rom, snaloader_rom_len);
            int r = fpga__upload_rom( snaloader_rom, snaloader_rom_len );
#endif
            if (r<0) {
                ESP_LOGE(TAG, "Error uploading ROM");
                return COMMAND_CLOSE_ERROR;
            }

            // Update len, remove the header size
            cmdt->romsize -= SNA_HEADER_SIZE;

            fpga__reset_to_custom_rom(true); // Also enable RETN hook

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

static char sna_error[31] = {0};

const char *sna__get_error_string()
{
    return sna_error;
}

static int sna__fix_and_write_sp(int fh)
{
    uint16_t sp;
    uint8_t v;
    int ret = 0;
    do {
        ret = fpga__read_extram(SNA_RAM_OFF_SPL);
        if (ret<0)
            break;
        sp = ret & 0xff;
        ret = fpga__read_extram(SNA_RAM_OFF_SPH);
        if (ret<0)
            break;
        sp |= (ret<<8);

        sp+=2;
        v = sp & 0xff;
        if (write(fh,&v, 1)!=1) {
            ret = -2;
            break;
        }
        v = (sp>>8) & 0xff;
        if (write(fh,&v, 1)!=1) {
            ret = -2;
            break;
        }
    } while (0);
    return ret;
}

static int sna__writefromextram(int fh, const uint32_t address)
{
    int r = fpga__read_extram(address);
    uint8_t v;
    if (r<0)
        return -1;
    v = r & 0xff;

    r = write(fh,&v, 1);
    if (r!=1) {
        ESP_LOGE(TAG, "Cannot write : %d : %s\n", r, strerror(errno));
        return -2;
    }
    return 0;
}
#define LOCAL_CHUNK_SIZE 512

static int sna__writefromextramblock(int fh, uint32_t address, int len)
{
    struct {
        struct extram_data_block b;
        uint8_t data[LOCAL_CHUNK_SIZE];
    } chunk;

    while (len>0) {
        int chunklen = len > LOCAL_CHUNK_SIZE ? LOCAL_CHUNK_SIZE :  len;

        int r = fpga__read_extram_block(address, &chunk.b, chunklen);

        if (r<0)
            return -1;

        if (write(fh, chunk.data, chunklen)!=chunklen) {
            ESP_LOGE(TAG, "Cannot write chunk : %d : %s\n", r, strerror(errno));
            return -2;
        }
        len -= chunklen;
        address += chunklen;
    }
    return 0;
}

int sna__save_from_extram(const char *file)
{
    char fpath[128];
    int ret = 0;
    sprintf(fpath,"/sdcard/%s", file);
    int fh = open(fpath, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fh<0) {
        ESP_LOGE(TAG,"Cannot open file %s: %s", fpath, strerror(errno));
        strcpy(sna_error,"Cannot open file");
        return -1;
    }

#define W(addr) if ((ret=sna__writefromextram(fh,addr))<0) break
    do {

        W(SNA_RAM_OFF_I);
        //   1        8      word   HL',DE',BC',AF'                        Check
        W(SNA_RAM_OFF_Lalt);
        W(SNA_RAM_OFF_Halt);
        W(SNA_RAM_OFF_Ealt);
        W(SNA_RAM_OFF_Dalt);
        W(SNA_RAM_OFF_Calt);
        W(SNA_RAM_OFF_Balt);
        W(SNA_RAM_OFF_Falt);
        W(SNA_RAM_OFF_Aalt);
        //   9        10     word   HL,DE,BC,IY,IX                         Check
        W(SNA_RAM_OFF_L);
        W(SNA_RAM_OFF_H);
        W(SNA_RAM_OFF_E);
        W(SNA_RAM_OFF_D);
        W(SNA_RAM_OFF_C);
        W(SNA_RAM_OFF_B);
        W(SNA_RAM_OFF_IYL);
        W(SNA_RAM_OFF_IYH);
        W(SNA_RAM_OFF_IXL);
        W(SNA_RAM_OFF_IXH);
        //   19       1      byte   Interrupt (bit 2 contains IFF2, 1=EI/0=DI)  Check
        W(SNA_RAM_OFF_IFF2);
        //   20       1      byte   R                                      Check
        W(SNA_RAM_OFF_R);
        //   21       4      words  AF,SP                                  Check
        W(SNA_RAM_OFF_F);
        W(SNA_RAM_OFF_A);
        // We need to fix SP.

        ret = sna__fix_and_write_sp(fh);
        if (ret<0)
            break;

        //W(SNA_RAM_OFF_SPL);
        //W(SNA_RAM_OFF_SPH);

        //   25       1      byte   IntMode (0=IM0/1=IM1/2=IM2)            Check
        W(SNA_RAM_OFF_IMM);
        //   26       1      byte   BorderColor (0..7, not used by Spectrum 1.7)  Check
        W(SNA_RAM_OFF_BORDER);
        //   27       49152  bytes  RAM dump 16384..65535

        ret = sna__writefromextramblock(fh, SNA_RAM_OFF_CHUNK1, SNA_RAM_CHUNK1_SIZE);
        if (ret<0) {
            break;
        }
        ret = sna__writefromextramblock(fh, SNA_RAM_OFF_CHUNK2, SNA_RAM_CHUNK2_SIZE);
        if (ret<0) {
            break;
        }
    } while (0);

    if (ret<0) {
        ESP_LOGE(TAG,"Cannot save to file");
        if (ret==-1) {
            ESP_LOGE(TAG,"FPGA comms failure");
            strcpy(sna_error,"FPGA Comms failure");
        } else{
            strcpy(sna_error,"Write error");
            ESP_LOGE(TAG,"Write error");
        }
        close(fh);
        return ret;
    }

    ret = close(fh);
    if (ret<0) {
        ESP_LOGE(TAG,"Error closing file");
        strcpy(sna_error,"Write error");
    }

    return ret;
}
