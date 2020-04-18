#include "fpga.h"
#include "spi.h"
#include "defs.h"
#include "dump.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "flash_pgm.h"

static spi_device_handle_t spi0_fpga;
static uint8_t latched_flags = 0;

static void fpga__init_spi()
{
    spi__init_device(&spi0_fpga, 20000000, PIN_NUM_CS);
}

unsigned fpga__read_id()
{
    uint8_t idbuf[5];

    idbuf[0] = 0x9F;
    idbuf[1] = 0xAA;
    idbuf[2] = 0x55;
    idbuf[3] = 0xAA;
    idbuf[4] = 0x55;


    int r = spi__transceive(spi0_fpga, idbuf, 5);

    if (r<0) {
        ESP_LOGE(TAG, "SPI transceive: error %d", r);
        return 0;
    }
    printf("FPGA id: ");
    dump__buffer(&idbuf[1], 4);
    printf("\r\n");

    return getbe32(&idbuf[1]);
}

int fpga__init()
{
    fpga__init_spi();
    fpga__read_id();
    fpga__set_trigger(FPGA_FLAG_TRIG_CMDFIFO_RESET | FPGA_FLAG_TRIG_RESOURCEFIFO_RESET);
    fpga__set_trigger(FPGA_FLAG_TRIG_INTACK);
    return 0;
}

uint8_t fpga__get_status()
{
    uint8_t buf[2];
    buf[0] = 0xDE;
    buf[1] = 0x00;
    spi__transceive(spi0_fpga, buf, sizeof(buf));
    return buf[1];
}

void fpga__set_clear_flags(uint8_t enable, uint8_t disable)
{
    uint8_t newflags = (latched_flags & ~disable) | enable;
    uint8_t buf[2];
    buf[0] = 0xEC;
    buf[1] = newflags;
    spi__transceive(spi0_fpga, buf, sizeof(buf));
    latched_flags = newflags;
}

void fpga__set_trigger(uint8_t trig)
{
    uint8_t buf[4];
    buf[0] = 0xEC;
    buf[1] = latched_flags;
    buf[2] = trig;
    buf[3] = 0x00; // Extra SPI clocks
    spi__transceive(spi0_fpga, buf, sizeof(buf));
}

void fpga__get_framebuffer(uint8_t *target)
{
    target[0] = 0xDF;
    target[1] = 0x00;
    target[2] = 0x00;
    target[3] = 0x00;
    int nlen = 4 + SPECTRUM_FRAME_SIZE;
//    int len = -1;

    spi__transceive(spi0_fpga, target, nlen);
    // Notify frame grabbed.
    target[0] = 0xEF;
    target[1] = 0x00;
    spi__transceive(spi0_fpga, target, 2);

}

void fpga__set_register(uint8_t reg, uint32_t value)
{
    uint8_t buf[6];
    buf[0] = 0xED;
    buf[1] = reg;
    buf[2] = (value >> 24)& 0xff;
    buf[3] = (value >> 16)& 0xff;
    buf[4] = (value >> 8)& 0xff;
    buf[5] = (value)& 0xff;

    spi__transceive(spi0_fpga, buf, sizeof(buf));
}

uint32_t fpga__get_register(uint8_t reg)
{
    uint8_t buf[6];
    buf[0] = 0xEE;
    buf[1] = reg;
    buf[2] = 0xAA;
    buf[3] = 0xAA;
    buf[4] = 0xAA;
    buf[5] = 0xAA;

    spi__transceive(spi0_fpga, buf, sizeof(buf));

    return getbe32(&buf[2]);
}

void fpga__set_capture_mask(uint32_t mask)
{
    fpga__set_register(REG_CAPTURE_MASK, mask);
}

void fpga__set_capture_value(uint32_t value)
{
    fpga__set_register(REG_CAPTURE_VAL, value);
}

uint32_t fpga__get_capture_status()
{
    uint8_t buf[6];
    buf[0] = 0xE2;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;

    spi__transceive(spi0_fpga, buf, sizeof(buf));
    uint32_t ret = getbe32( &buf[2] );
    return ret;
}

int fpga__get_captures(uint8_t *target)
{
    target[0] = 0xE0;
    target[1] = 0x00;
    int nlen = 2+ (5 * 2048);
    //int len = -1;
    spi__transceive(spi0_fpga, target, nlen);
    return nlen;
}

int fpga__upload_rom_chunk(uint16_t offset, uint8_t *buffer_sub3, unsigned len)
{
    // Upload chunk
    buffer_sub3[0] = 0xE1;
    buffer_sub3[1] = (offset>>8) & 0xFF;
    buffer_sub3[2] = offset & 0xFF;

    {
        dump__buffer(buffer_sub3, 8);
    }

    spi__transceive(spi0_fpga, buffer_sub3, 3 + len);

    return len;
}

int fpga__reset_to_custom_rom(bool activate_retn_hook)
{
    ESP_LOGI(TAG, "Resetting spectrum (to custom ROM)");

    fpga__set_flags(FPGA_FLAG_RSTSPECT | FPGA_FLAG_CAPCLR);

    fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_ON);

    if (activate_retn_hook) {
        fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMONRETN);
    }

    vTaskDelay(2 / portTICK_RATE_MS);
    fpga__clear_flags(FPGA_FLAG_RSTSPECT);

    ESP_LOGI(TAG, "Reset completed");
    return 0;
}

int fpga__load_resource_fifo(const uint8_t *data, unsigned len, int timeout)
{
    printf("Load res: ");
    dump__buffer(data,len);
    printf("\n");

#define LOCAL_CHUNK_SIZE 512
    uint8_t txbuf[LOCAL_CHUNK_SIZE+1];

    do {
        uint8_t stat = fpga__get_status();
        unsigned maxsize;
        if (stat & FPGA_STATUS_RESFIFO_FULL) {
            maxsize=0;
        } else if (stat & FPGA_STATUS_RESFIFO_QQQFULL) {
            maxsize = 0;
        } else if (stat & FPGA_STATUS_RESFIFO_HFULL) {
            maxsize = (FPGA_RESOURCE_FIFO_SIZE/4) -1;
        } else if (stat & FPGA_STATUS_RESFIFO_QFULL) {
            maxsize = (FPGA_RESOURCE_FIFO_SIZE/2) -1;
        } else {
            maxsize = (FPGA_RESOURCE_FIFO_SIZE*4/3)-1;
        }

        ESP_LOGI(TAG,"Resource FIFO stat %02x avail size %d, len %d", stat, maxsize, len);

        if (maxsize > LOCAL_CHUNK_SIZE)
            maxsize = LOCAL_CHUNK_SIZE;

        if (maxsize>len)
            maxsize = len;

        ESP_LOGI(TAG,"Resource FIFO upload size %d", maxsize);

        // Upload chunk to resource fifo.
        if (maxsize>0) {
            memset(txbuf,0xF7, sizeof(txbuf));
            txbuf[0] = 0xE3;
            memcpy(&txbuf[1], data, maxsize);
            spi__transceive(spi0_fpga, txbuf, maxsize+1);

            data+=maxsize;
            len-=maxsize;

        } else {
            // Delay.
            vTaskDelay(100 / portTICK_RATE_MS);
            if (timeout>0) {
                timeout--;
            } else {
                ESP_LOGI(TAG, "Timeout waiting for FIFO room");
                return -1;
            }
        }
    } while (len);
    return 0;
}

int fpga__upload_rom(const uint8_t *buffer, unsigned len)
{
    uint16_t offset = 0;
    uint8_t tbuf[67];
    ESP_LOGI(TAG, "Uploading ROM, %d bytes", len);
    do {
        // Upload chunk
        tbuf[0] = 0xE1;
        tbuf[1] = (offset>>8) & 0xFF;
        tbuf[2] = offset & 0xFF;

        int llen = len>64?64:len;
        memcpy(&tbuf[3], buffer, llen);
        spi__transceive(spi0_fpga, tbuf, llen+3);
        buffer+=llen;
        len-=llen;
        offset+=llen;
    } while (len);

    return 0;
}


#define FLASH_SECTOR_SIZE 65536
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_MASK (FLASH_SECTOR_SIZE-1)

int fpga__startprogram(fpga_program_state_t *state)
{
    state->first_unerased_address = 0;
    state->current_write_address = 0;
    unsigned id = fpga_pgm__read_id();
    ESP_LOGI(TAG, "FPGA Flash ID: %08x", id);
    state->flashid = id;
    if ((id & 0xFFFFFF) == 0x202015) {
        return 0;
    }
    return -1;
}

/*
 Case 1: unerased=0    last=65535 - sector to erase 0
 Case 1: unerased=65536 last=65535 - sector to erase: none
 Case 1: last=65534 end=65535 - sector to erase: 0
 */

static inline int fpga__erase_until(fpga_program_state_t*state, int last_address)
{
    int first_sector_to_erase = (state->first_unerased_address & ~FLASH_SECTOR_MASK);
    int last_sector_to_erase = (last_address+FLASH_SECTOR_MASK) & ~FLASH_SECTOR_MASK;

    ESP_LOGI(TAG,"First unerased address 0x%08x start at 0x%08x mask 0x%08x",
             state->first_unerased_address,
             first_sector_to_erase,
             FLASH_SECTOR_MASK
            );

    while (last_sector_to_erase > first_sector_to_erase) {
        if (flash_pgm__erase_sector_address(first_sector_to_erase)<0)
            return -1;
        first_sector_to_erase += FLASH_SECTOR_SIZE;
        state->first_unerased_address = first_sector_to_erase;
        ESP_LOGI(TAG,"Erased, first unerased address 0x%08x", first_sector_to_erase);
    }
    return 0;
}

int fpga__program( fpga_program_state_t *state, const uint8_t *data, unsigned len)
{
    int last_address = (state->current_write_address + len)-1;

    if (last_address >= state->first_unerased_address) {
        ESP_LOGI(TAG,"Erasing up to 0x%08x", last_address);
        fpga__erase_until(state, last_address);
    }

    while (len)  {
        // max 256 bytes page. We may need to do partial writes here.
        int page_used = state->current_write_address & (FLASH_PAGE_SIZE-1);
        int page_avail = FLASH_PAGE_SIZE - page_used;

        int pgmsize = len > page_avail ? page_avail : len;

        flash_pgm__program_page( state->current_write_address, data, pgmsize);
        data += pgmsize;
        state->current_write_address += pgmsize;
        len-=pgmsize;
    }

    return -1;
}

int fpga__finishprogram( fpga_program_state_t *state)
{
    fpga__trigger_reconfiguration();
    vTaskDelay(100 / portTICK_RATE_MS);
    unsigned id = fpga__read_id();
    ESP_LOGI(TAG, "FPGA ID: 0x%08x", id);
    return 0;
}

void fpga__trigger_reconfiguration()
{
    gpio_set_level(PIN_NUM_NCONFIG, 0);
    vTaskDelay(1 / portTICK_RATE_MS);
    gpio_set_level(PIN_NUM_NCONFIG, 1);
}


int fpga__read_command_fifo()
{
    uint8_t buf[3];
    buf[0] = 0xFB;
    buf[1] = 0x00;
    buf[2] = 0x00;
    spi__transceive(spi0_fpga, buf, 3);
    printf("Cmd fifo state: ");
    dump__buffer(buf, 3);
    printf("\n");
    if (buf[1]==0xff) {
        return -1;
    }
    return buf[2];
}






