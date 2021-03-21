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
#include "sna_z80.h"
#include "dump.h"
#include "sna_defs.h"
#include <stdlib.h>
#include "fileaccess.h"
#include "minmax.h"
#include <assert.h>

#define TEST_WRITES

#undef TEST_ROM

extern unsigned char snaloader_rom[];
extern unsigned int snaloader_rom_len;

#ifdef TEST_ROM
extern unsigned char INTZ_ROM[];
extern unsigned int INTZ_ROM_len;
#endif


int sna__z80_handle_block_compress(int fd, int len, unsigned memaddress);
int sna__z80_handle_block_v2(int fd);


static void sna__prepare(void);
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
#if 0
    unsigned remain = cmdt->romsize - cmdt->romoffset;

    if (remain<cmdt->len) {
        // Too much data, complain
        ESP_LOGE(TAG, "ROM: expected max %d but got %d bytes", remain, cmdt->len);
        return COMMAND_CLOSE_ERROR;
    }

    if (header_seen == false) {

        if ( cmdt->len > SNA_HEADER_SIZE ) {
#ifndef TEST_ROM
            sna_apply_relocs_mem(cmdt->rx_buffer, snaloader_rom, 0x0080);
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
#endif
    return COMMAND_CLOSE_ERROR;
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
    ESP_LOGI(TAG, "SNA: reading from EXTRAM");

    do {
        ESP_LOGI(TAG, "SNA: reading SPL from %06x", SNA_RAM_OFF_SPL);
        ret = fpga__read_extram(SNA_RAM_OFF_SPL);
        if (ret<0)
            break;

        ESP_LOGI(TAG, "EXTRAM: %06x : %02x", SNA_RAM_OFF_SPL, ret);

        sp = ret & 0xff;
        ESP_LOGI(TAG, "SNA: reading SPH from %06x", SNA_RAM_OFF_SPH);
        ret = fpga__read_extram(SNA_RAM_OFF_SPH);
        if (ret<0)
            break;
        sp |= (ret<<8);
        ESP_LOGI(TAG, "EXTRAM: %06x : %02x", SNA_RAM_OFF_SPH, ret);

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

    ESP_LOGI(TAG, "EXTRAM: %06x : %02x", address, v);

    r = write(fh,&v, 1);
    if (r!=1) {
        ESP_LOGE(TAG, "Cannot write : %d : %s", r, strerror(errno));
        return -2;
    }
    return 0;
}
#define LOCAL_CHUNK_SIZE 512

static int sna__writefromextramblock(int fh, uint32_t address, int len)
{
    union {
        uint8_t w8[LOCAL_CHUNK_SIZE];
        uint32_t w32[LOCAL_CHUNK_SIZE/4];
    } data;

    while (len>0) {
        int chunklen = MIN(len, LOCAL_CHUNK_SIZE);

        int r = fpga__read_extram_block(address, &data.w32[0], chunklen);

        if (r<0)
            return -1;

        if (write(fh, &data.w8[0], chunklen)!=chunklen) {
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
    int fh = __open(fpath, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    ESP_LOGI(TAG, "Opened file %s", fpath);
    if (fh<0) {
        ESP_LOGE(TAG,"Cannot open file %s: %s", fpath, strerror(errno));
        strcpy(sna_error,"Cannot open file");
        return -1;
    }

    // DEBUG ONLY
#if 0
    ESP_LOGI(TAG,"First 8 bytes of extram: ");
    fpga__read_extram_block(0x0, (uint8_t*)fpath, 8);
    dump__buffer((uint8_t*)fpath,8);
#endif

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
        ESP_LOGI(TAG, "Writing block data 1");
        ret = sna__writefromextramblock(fh, SNA_RAM_OFF_CHUNK1, SNA_RAM_CHUNK1_SIZE);
        if (ret<0) {
            break;
        }
        ESP_LOGI(TAG, "Writing block data 2");
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

#undef LOCAL_CHUNK_SIZE
#ifdef __linux__
#define LOCAL_CHUNK_SIZE 2048
#else
#define LOCAL_CHUNK_SIZE 64
#endif

int sna__load_sna_extram(int fd)
{
    union {
        uint8_t w8[LOCAL_CHUNK_SIZE];
        uint32_t w32[LOCAL_CHUNK_SIZE/4];
    } chunk;

    uint8_t header[SNA_HEADER_SIZE];

    unsigned togo = 0xC000; // 48KB
    unsigned extram_address = SNA_EXTRAM_ADDRESS;

    // Read in SNA header
    if (read(fd, header, sizeof(header))!=sizeof(header)) {
        ESP_LOGE(TAG,"Cannot read SNA header: %s", strerror(errno));
        return -1;
    }

    sna_apply_relocs_fpgarom(header, ROM_PATCHED_SNALOAD_ADDRESS);

    // Now, load rest of file to memory


    extram_address = SNA_EXTRAM_ADDRESS;

    while (togo) {
        int chunksize = MIN(togo, LOCAL_CHUNK_SIZE);
        int r = read(fd, &chunk.w8[0], chunksize);
        if (r!=chunksize) {
            ESP_LOGE(TAG, "Short read from file: %s", strerror(errno));
            return -1;
        }
#ifdef TEST_WRITES
        uint8_t rdc[LOCAL_CHUNK_SIZE];
        memcpy(rdc, &chunk.w8[0], chunksize);
        // RDC now stores original data.
#endif
        if (fpga__write_extram_block(extram_address, &chunk.w8[0], chunksize)<0) {
            ESP_LOGE(TAG, "Error writing to external FPGA RAM");
            return -1;
        }
#ifdef TEST_WRITES
        if (fpga__read_extram_block(extram_address, &chunk.w32[0], chunksize)<0) {
            ESP_LOGE(TAG, "Error reading from external FPGA RAM");
            return -1;
        }
        if (memcmp(rdc,&chunk.w8[0],chunksize)!=0) {

            ESP_LOGE(TAG, "ERROR comparing data!!! address:%08x len=%d", extram_address, chunksize);
            dump__buffer(rdc, chunksize);
            dump__buffer(&chunk.w8[0], chunksize);
            memcpy(&chunk.w8[0], rdc, sizeof(chunk) );
            if (fpga__write_extram_block(extram_address, &chunk.w8[0], chunksize)<0) {
                ESP_LOGE(TAG, "Error writing to external FPGA RAM");
                return -1;
            }
            return -1;
        }
#endif

        extram_address += chunksize;
        togo -= chunksize;
    }

    return 0;
}

static int sna__z80_init_pages()
{
    const uint8_t pagevals[9] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    return fpga__write_extram_block(SNA_Z80_EXTRAM_PAGE_ALLOCATION_ADDRESS,
                                    pagevals,
                                    sizeof(pagevals));
}

static int sna__z80_set_page_at_index(int index, uint8_t page)
{
    return fpga__write_extram(SNA_Z80_EXTRAM_PAGE_ALLOCATION_ADDRESS + index,
                              page);
}

static int sna__z80_set_block_at_index(int index, uint8_t block)
{
    if (block<3)
        return -1;
    block-=3;
    if (block>7)
        return -1;
    return sna__z80_set_page_at_index(index, block);
}

static int sna__z80_apply_ext_data(sna_z80_ext_header_t *ext)
{
    int r = -1;
    do {
        r = fpga__write_extram(SNA_Z80_EXTRAM_LAST7FFD_ADDRESS, ext->out1);
        if (r<0)
            break;
        r = fpga__write_extram_block(SNA_Z80_EXTRAM_YM_REGISTERCONTENT_ADDRESS,
                                     &ext->soundchipregs[0],
                                     sizeof(ext->soundchipregs)
                                    );
        if (r<0)
            break;
        r = fpga__write_extram(SNA_Z80_EXTRAM_YM_LASTREGISTER_ADDRESS, ext->soundchipregno);
    } while (0);
    return r;
}



snatype_t sna__load_z80_extram(int fd)
{
    //char fullfile[128];
    //uint8_t chunk[LOCAL_CHUNK_SIZE];
    sna_z80_main_header_t header;
    sna_z80_ext_header_t ext;

    snatype_t type = SNAPSHOT_ERROR;

    //unsigned togo = 0xC000; // 48KB
    //unsigned extram_address = SNA_EXTRAM_ADDRESS;

    ESP_LOGI(TAG, "Inspecting Z80 snapshot %d", sizeof(header));
#ifdef __linux__
    assert( sizeof(header)==30 );
#endif

    // Read in Z80 header
    if (read(fd, &header, sizeof(header))!=sizeof(header)) {
        ESP_LOGE(TAG,"Cannot read Z80 header: %s", strerror(errno));
        return -1;
    }
    // Check if PC is zero. If yes, we have more data
    if (header.pc == 0x0000) {
        // Read LEN first.
        if (read(fd, &ext, sizeof(ext.len))!=sizeof(ext.len)) {
            ESP_LOGE(TAG,"Cannot read Z80 ext header: %s", strerror(errno));
            return -1;
        }
        // Read remaing
        // Sanity check: we support v1, v2 and v3 only.
        unsigned toread = ext.len;
        switch (toread) {
        case 23:
            type = SNAPSHOT_Z80_V2;
            ESP_LOGI(TAG, "Loading Z80 snapshot V2");
            break;
        case 54: /* Fall-through */
            type = SNAPSHOT_Z80_V3;
            ESP_LOGI(TAG, "Loading Z80 snapshot V3");
            break;
        case 55:
            type = SNAPSHOT_Z80_V3PLUS;
            ESP_LOGI(TAG, "Loading Z80 snapshot V3+");
            break;
        default:
            ESP_LOGE(TAG,"Unknown Z80 ext header len: %04x",toread);
            break;
        }
        if (type!=SNAPSHOT_ERROR) {
            if (read(fd, &ext.pc, toread)!=toread) {
                ESP_LOGE(TAG,"Cannot read Z80 ext2 header: %s", strerror(errno));
                return -1;
            }
            // Apply ext data.
            sna__z80_apply_ext_data(&ext);
        }
    } else {
        type = SNAPSHOT_Z80_V1;
        ESP_LOGI(TAG, "Loading Z80 snapshot V1");
    }

    if (type==SNAPSHOT_ERROR)
        return type;

    sna__z80_init_pages();

    sna_z80_apply_relocs_fpgarom(&header,
                                 &ext,
                                 ROM_PATCHED_SNALOAD_ADDRESS);


    switch (type) {
    case SNAPSHOT_Z80_V1:
        if (header.compressed) {
            ESP_LOGI(TAG, "Compressed Z80 snapshot V1");
            if (sna__z80_handle_block_compress(fd, -1, SNA_Z80_EXTRAM_BASE_PAGE_ADDRESS)<0)
                type = SNAPSHOT_ERROR;
            // Set up page pointers.

            /*
                    48K             128K
             3      -               page 0   
             4      8000-bfff       page 1   
             5      c000-ffff       page 2   
             6      -               page 3   
             7      -               page 4   
             8      4000-7fff       page 5   
             9      -               page 6   
             10     -               page 7
              */
            sna__z80_set_page_at_index(0, 5);
            sna__z80_set_page_at_index(1, 1);
            sna__z80_set_page_at_index(2, 2);
        } else {                       
            type = SNAPSHOT_ERROR; // TBD
        }
        break;
    case SNAPSHOT_Z80_V2:    /* Fall-through */
    case SNAPSHOT_Z80_V3:    /* Fall-through */
    case SNAPSHOT_Z80_V3PLUS:
        if (sna__z80_handle_block_v2(fd)<0)
            type = SNAPSHOT_ERROR;
        break;
    default:
        break;
    }

    return type;
}

snatype_t sna__load_snapshot_extram(const char *file)
{
    char fullfile[128];
    snatype_t type = SNAPSHOT_ERROR;

    fullpath(file, fullfile, sizeof(fullfile)-1);

    const char *ext = get_file_extension(file);

    if (!ext)
        return type;

    if (ext_match(ext,"sna")) {
        type = SNAPSHOT_SNA;
    } else if (ext_match(ext,"z80")) {
        type = SNAPSHOT_Z80_V1;  // Will be updated later
    }

    if (type==SNAPSHOT_ERROR)
        return type;

    int fd = __open(fullfile, O_RDONLY);
    if (fd<0) {
        ESP_LOGE(TAG,"Cannot open '%s': %s", fullfile, strerror(errno));
        return -1;
    }

    switch (type) {
    case SNAPSHOT_SNA:
        type = sna__load_sna_extram(fd);
        break;
    case SNAPSHOT_Z80_V1:
        type = sna__load_z80_extram(fd);
        break;
    default:
        break;
    }
    close(fd);

    return type;
}



void z80_decomp_init(z80_decomp_t *decomp, uint8_t version,
                     int (*writer)(void *arg, uint8_t val),
                     void *writer_arg)
{
    ESP_LOGI(TAG,"Z80 decomp init for v%d", version);
    decomp->state = Z80D_RAW;
    decomp->bytes_output = 0;
    decomp->version = version;
    decomp->writer = writer;
    decomp->writer_arg = writer_arg;
}



int z80_decompress(z80_decomp_t *decomp, uint8_t *source, int len)
{
#define OUTPUT(x) do { decomp->bytes_output++; \
    decomp->writer(decomp->writer_arg, x); \
} while (0)

    while (len--) {
        uint8_t v = *source++;
        switch (decomp->state) {
        case Z80D_RAW:
            if (v==0xED) {
                decomp->state = Z80D_ED0;
                break;
            }
            // Check EOB marker
            if ((decomp->version==1) && v==0x00) {
                decomp->state = Z80D_ZERO;
                break;
            }
            //ESP_LOGI("Z80", "RAW %02x %d", v, decomp->bytes_output);
            OUTPUT(v);
            break;
        case Z80D_ED0:
            if (v==0xED) {
                decomp->state = Z80D_ED1;
                break;
            }
            //ESP_LOGI("Z80", "RAW 0xED ! %d", decomp->bytes_output);
            OUTPUT(0xED);
            //ESP_LOGI("Z80", "RAW %02x %d", v, decomp->bytes_output);
            OUTPUT(v);
            decomp->state = Z80D_RAW;
            break;

        case Z80D_ED1:
            decomp->repcnt = v;
            decomp->state = Z80D_LEN;
            break;

        case Z80D_ZERO:
            if (v==0xED) {
                decomp->state = Z80D_ZED0;
                break;
            }
            OUTPUT(0x0);
            OUTPUT(v);
            decomp->state = Z80D_RAW;
            break;

        case Z80D_ZED0:
            if (v==0xED) {
                decomp->state = Z80D_ZED1;
                break;
            }
            OUTPUT(0x0);
            OUTPUT(0xED);
            OUTPUT(v);
            decomp->state = Z80D_RAW;
            break;

        case Z80D_ZED1:
            if (v==0x00) {
                decomp->state = Z80D_EOF;  // Finshed
                break;
            }
            // This is a repeat.
            OUTPUT(0x0);
            decomp->repcnt = v;
            decomp->state = Z80D_LEN;
            break;

        case Z80D_LEN:
            //ESP_LOGI("Z80", "REP %02x %d %d", v, decomp->repcnt, decomp->bytes_output);

            while (decomp->repcnt--)
                OUTPUT(v);
            decomp->state = Z80D_RAW;
            break;

        case Z80D_EOF:
            ESP_LOGI(TAG,"Z80: data post EOF");
            return -1; // Error
        }
    }
    return 0;
}

static int z80_decompress_is_eof(z80_decomp_t *decomp)
{
    return decomp->state == Z80D_EOF;
}

static int z80_decompress_is_idle(z80_decomp_t *decomp)
{
    return decomp->state == Z80D_RAW;
}


int sna__z80_handle_block_v2(int fd)
{
    sna_z80_block_header_t hdr;
    unsigned address = SNA_Z80_EXTRAM_BASE_PAGE_ADDRESS;
    int index = 0;
    do {
        // TBC: we do not know how many blocks we have. So read until EOF.
        assert(sizeof(hdr)==3);

        int r = read(fd, &hdr, sizeof(hdr));

        if (r==0)
            break;

        if (r!=sizeof(hdr)) {
            ESP_LOGE(TAG,"Cannot read Z80 block header: %s", strerror(errno));
            return -1;
        }

        if (hdr.len == 0xffff) {
            // Not compressed.  TODO
            return -1;
        } else {
            r = sna__z80_handle_block_compress(fd, hdr.len, address);

            if (r<0)
                return r;

            if (r!=16384) {
                ESP_LOGI(TAG,"Z80: short block %d (original len %d for block %d)",r, hdr.len, hdr.block);
                return -1;
            }

            // Set up page.

            r = sna__z80_set_block_at_index(index, hdr.block);
            if (r<0)
                return r;
            // increment address
            address += 16384; // TBC: should we check for actual size?
            //

        }
        index++;

    } while (1);
    return 0;
}

struct z80writebuffer {
    uint8_t buffer[128];
    unsigned memaddress;
//    unsigned size;
    unsigned off;
};


int sna__z80_buffered_flush(struct z80writebuffer *wbuf)
{
    if (wbuf->off) {
        if (fpga__write_extram_block(wbuf->memaddress, wbuf->buffer, wbuf->off)<0)
            return -1;
        wbuf->memaddress+=wbuf->off;
        wbuf->off = 0;
    }
    return 0;
}

int sna__z80_buffered_writer(void *arg, uint8_t val)
{
    struct z80writebuffer *wbuf = (struct z80writebuffer*)arg;

    wbuf->buffer[wbuf->off++] = val;

    if (wbuf->off==sizeof(wbuf->buffer)) {
        return sna__z80_buffered_flush(wbuf);
    }
    return 0;
}

int sna__z80_handle_block_compress(int fd, int len, unsigned memaddress)
{
    uint8_t chunk[LOCAL_CHUNK_SIZE];
    z80_decomp_t decomp;
    unsigned toread;
    struct z80writebuffer wbuf;

    wbuf.off = 0;
    wbuf.memaddress = memaddress;

    ESP_LOGI(TAG,"Z80: handling block compress len %d", len);

    if (len<0) {
        z80_decomp_init(&decomp, 1, &sna__z80_buffered_writer, &wbuf);
    } else {
        z80_decomp_init(&decomp, 2, &sna__z80_buffered_writer, &wbuf);
    }

    do {
        if (len>0) {
            toread = len > LOCAL_CHUNK_SIZE ? LOCAL_CHUNK_SIZE: len;


            if (read(fd, chunk, toread)!=toread) {
                ESP_LOGE(TAG,"Cannot read Z80 block: %s", strerror(errno));
                return -1;
            }
        } else {
            toread = LOCAL_CHUNK_SIZE;
            toread = read(fd, chunk, toread);
        }

        if (z80_decompress(&decomp, chunk, toread)<0) {
            ESP_LOGE(TAG,"Z80: error decompressing");
            return -1;
        }

        if (len>0)
            len -= toread;
        // Check if EOF for V1
        if (z80_decompress_is_eof(&decomp)) {
            break;
        }

    } while (len || len==-1);

    sna__z80_buffered_flush(&wbuf);


    ESP_LOGI(TAG,"Z80: decompressed %d", decomp.bytes_output);

    if (len<0) {
    } else {
        if (!z80_decompress_is_idle(&decomp)) {
            ESP_LOGE(TAG, "Z80: Decompressor not idle!");
            return -1;
        }
    }

    return decomp.bytes_output;
}


