#include "fpga.h"
#include "spi.h"
#include "defs.h"
#include "dump.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "flash_pgm.h"
#include "gpio.h"
#include "esp_spi_flash.h"
#include <alloca.h>
#include "byteops.h"
#include "minmax.h"
#include "fileaccess.h"
#include "string.h"
#include "errno.h"
#include "bitrev.h"

#define FPGA_BINARYFILE "/spiffs/fpga0.rbf"

static spi_device_handle_t spi0_fpga;

static fpga_flags_t latched_flags = 0;
static uint32_t config1_latch = 0;
static uint32_t fpga_id;


uint32_t fpga__id(void)
{
    return fpga_id;
}

static void fpga__init_spi()
{
    spi__init_device(&spi0_fpga, 10000000, PIN_NUM_CS);
}

static int fpga__issue_read(uint8_t cmd, uint8_t *buf, unsigned size)
{
    return spi__transceive_cmd8_addr8(spi0_fpga, cmd, 0xFF, buf, size);
}

static int fpga__issue_read_addr8(uint8_t cmd, uint8_t addr, uint8_t *buf, unsigned size)
{
    return spi__transceive_cmd8_addr16(spi0_fpga, cmd, (uint16_t)addr<<8, buf, size);
}

static int fpga__issue_read_block(uint8_t cmd, uint8_t *buf, uint8_t size)
{
    // reuse addr16 as length indicator.
    uint8_t addr = size;
    return fpga__issue_read_addr8(cmd, addr, buf, size);
}

static int fpga__issue_read_addr16(uint8_t cmd, uint16_t addr, uint8_t *buf, unsigned size)
{
    return spi__transceive_cmd8_addr24(spi0_fpga, cmd, (uint32_t)addr<<8, buf, size);
}

static int fpga__issue_read_addr24(uint8_t cmd, uint32_t addr, uint8_t *buf, unsigned size)
{
    return spi__transceive_cmd8_addr32(spi0_fpga, cmd, (uint32_t)addr<<8, buf, size);
}

static int fpga__issue_write(uint8_t cmd, const uint8_t *buf, unsigned size)
{
    return spi__transmit_cmd8(spi0_fpga, cmd, buf, size);
}

static int fpga__issue_write_addr24(uint8_t cmd, uint32_t address, const uint8_t *buf, unsigned size)
{
    return spi__transmit_cmd8_addr24(spi0_fpga, cmd, address, buf, size);
}

static int fpga__issue_write_addr16(uint8_t cmd, uint16_t address, const uint8_t *buf, unsigned size)
{
    return spi__transmit_cmd8_addr16(spi0_fpga, cmd, address, buf, size);
}

static unsigned fpga__read_id()
{
    uint8_t idbuf[4];

    int r = fpga__issue_read(FPGA_SPI_CMD_READ_ID, idbuf, 4);

    if (r<0) {
        ESP_LOGE(TAG, "SPI transceive: error %d", r);
        return 0;
    }
    printf("FPGA id: ");
    dump__buffer(&idbuf[0], 4);
    printf("\r\n");

    return extractbe32(&idbuf[0]);
}

static int fpga__configurefromflash()
{
#ifndef __linux__
    int fh;
    int r = -1;
    struct stat st;
    do {
        r = __lstat(FPGA_BINARYFILE, &st);
        if (r<0) {
            ESP_LOGE(TAG, "Cannot stat FPGA image");
            break;
        }
        fh = __open(FPGA_BINARYFILE, O_RDONLY);
        if (fh<0) {
            ESP_LOGE(TAG, "Cannot open FPGA image");
            break;
        }
        r = fpga__passiveserialconfigure_fromfile( fh, st.st_size );
    } while (0);

    if (r!=0) {
        ESP_LOGE(TAG, "Cannot load FPGA binary!");
    }
    return r;
#else
    return 0;
#endif
}


int fpga__init()
{
    fpga__init_spi();

    if (fpga__configurefromflash()<0)
        return -1;
    do {
        fpga_id = fpga__read_id();
    } while ((fpga_id & 0xff) == 0xff);
    fpga__set_trigger(FPGA_FLAG_TRIG_CMDFIFO_RESET | FPGA_FLAG_TRIG_RESOURCEFIFO_RESET);
    fpga__set_trigger(FPGA_FLAG_TRIG_INTACK);
    fpga__set_clear_flags(FPGA_FLAG_ENABLE_INTERRUPT, FPGA_FLAG_RSTSPECT);

    return 0;
}

uint8_t fpga__get_status()
{
    uint8_t buf[1];

    ESP_ERROR_CHECK(fpga__issue_read(FPGA_SPI_CMD_READ_STATUS, buf, 1));

    return buf[0];
}

uint16_t fpga__get_spectrum_pc()
{
    uint8_t buf[2];
    spi__transceive(spi0_fpga, buf, sizeof(buf));
    ESP_ERROR_CHECK(fpga__issue_read(FPGA_SPI_CMD_READ_PC, buf, 2));
    return extractbe16(&buf[2]);
}

void fpga__set_clear_flags(fpga_flags_t enable, fpga_flags_t disable)
{
    uint8_t buf[3];
    fpga_flags_t newflags = (latched_flags & ~disable) | enable;

    buf[0] = newflags&0xff;
    buf[1] = 0x00;
    buf[2] = newflags>>8;

    ESP_ERROR_CHECK(fpga__issue_write(FPGA_SPI_CMD_WRITE_FLAGS, buf, sizeof(buf)));

    latched_flags = newflags;
}


void fpga__set_trigger(uint8_t trig)
{
    uint8_t buf[3];
    buf[0] = latched_flags & 0xff;
    buf[1] = trig;
    buf[2] = latched_flags >> 8;

    ESP_ERROR_CHECK(fpga__issue_write(FPGA_SPI_CMD_WRITE_FLAGS, buf, sizeof(buf)));
}

void fpga__get_framebuffer(uint8_t *target)
{
    uint8_t dummy[1] = { 0x00 };
    ESP_ERROR_CHECK(fpga__issue_read_addr16(FPGA_SPI_CMD_READ_VIDEOMEM, 0x0000, target, SPECTRUM_FRAME_SIZE));
    // Notify frame grabbed.
    ESP_ERROR_CHECK(fpga__issue_write(FPGA_SPI_CMD_SETEOF, dummy, sizeof(dummy)));
}

void fpga__set_register(uint8_t reg, uint32_t value)
{
    uint8_t buf[5];
    buf[0] = reg;
    buf[1] = (value >> 24)& 0xff;
    buf[2] = (value >> 16)& 0xff;
    buf[3] = (value >> 8)& 0xff;
    buf[4] = (value)& 0xff;

    ESP_ERROR_CHECK(fpga__issue_write(FPGA_SPI_CMD_WRITE_REG32, buf, sizeof(buf)));
}

uint32_t fpga__get_register(uint8_t reg)
{
    uint8_t buf[4];

    ESP_ERROR_CHECK(fpga__issue_read_addr8(FPGA_SPI_CMD_READ_REG32, reg, buf, sizeof(buf)));

    return extractbe32(&buf[0]);
}

int fpga__upload_rom_chunk(uint32_t baseaddress, uint16_t offset, uint8_t *buffer_sub3, unsigned len)
{
    if (fpga__write_extram_block(baseaddress + (uint32_t)offset, &buffer_sub3[3], len)<0) {
        ESP_LOGE(TAG, "Cannot write ROM block");
        return -1;
    }
    return len;
}

int fpga__get_reset_time()
{
    return 500;
}

int fpga__reset_to_custom_rom(int romno, bool activate_retn_hook)
{
    ESP_LOGI(TAG, "Resetting spectrum (to custom ROM)");

    fpga__set_flags(FPGA_FLAG_RSTSPECT | FPGA_FLAG_CAPCLR);

    fpga__set_rom(romno);

    fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_ON);

    if (activate_retn_hook) {
        fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMONRETN);
    }

    vTaskDelay(fpga__get_reset_time() / portTICK_RATE_MS);
    fpga__clear_flags(FPGA_FLAG_RSTSPECT);

    ESP_LOGI(TAG, "Reset completed");
    return 0;
}

int fpga__reset_spectrum()
{
    ESP_LOGI(TAG, "Resetting spectrum");

    fpga__set_flags(FPGA_FLAG_RSTSPECT);

    fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_OFF | FPGA_FLAG_TRIG_FORCENMI_OFF);

    vTaskDelay(fpga__get_reset_time() / portTICK_RATE_MS);
    fpga__clear_flags(FPGA_FLAG_RSTSPECT);

    ESP_LOGI(TAG, "Reset completed");
    return 0;
}

int fpga__load_resource_fifo(const uint8_t *data, unsigned len, int timeout)
{

#define LOCAL_CHUNK_SIZE 512
//    uint8_t txbuf[LOCAL_CHUNK_SIZE+1];

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

        //ESP_LOGI(TAG,"Resource FIFO stat %02x avail size %d, len %d", stat, maxsize, len);

        if (maxsize > LOCAL_CHUNK_SIZE)
            maxsize = LOCAL_CHUNK_SIZE;

        if (maxsize>len)
            maxsize = len;

        //ESP_LOGI(TAG,"Resource FIFO upload size %d", maxsize);

        // Upload chunk to resource fifo.
        if (maxsize>0) {
            //txbuf[0] = 0xE3;
            //memcpy(&txbuf[1], data, maxsize);
            //spi__transceive(spi0_fpga, txbuf, maxsize+1);
            fpga__issue_write(FPGA_SPI_CMD_WRITE_RESOURCEFIFO, data, maxsize);
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

int fpga__write_rom(unsigned offset, uint8_t val)
{
    ESP_LOGI(TAG,"Patching rom @ 0x%08x: 0x%02x\n", offset, val);
    return fpga__write_extram(offset, val);
}

int fpga__upload_rom(uint32_t baseaddress, const uint8_t *buffer, unsigned len)
{
    uint16_t offset = 0;
#ifdef __linux
    uint8_t tbuf[4096];
#else
    uint8_t tbuf[64];
#endif
    ESP_LOGI(TAG, "Uploading ROM, %d bytes", len);
    do {
        int llen = MIN(len,sizeof(tbuf));

        if (fpga__write_extram_block(baseaddress+(uint32_t)offset, buffer, llen)<0)
            return -1;
#if 1
        if (fpga__read_extram_block(baseaddress+(uint32_t)offset, tbuf, llen)<0)
            return -1;

        if (memcmp(tbuf, buffer, llen)!=0) {
            ESP_LOGE(TAG,"ERROR comparing ROM contents\n");
            dump__buffer(buffer, llen);
            dump__buffer(tbuf, llen);

            memset(tbuf, 0, sizeof(tbuf));

            if (fpga__read_extram_block(baseaddress+(uint32_t)offset, tbuf, llen)<0)
                return -1;

            dump__buffer(tbuf, llen);

            return -1;
        }

        //ESP_LOGI(TAG, "Chunk %d at 0x%08x", llen, (uint32_t)offset);
#endif
        buffer+=llen;
        len-=llen;
        offset+=llen;
    } while (len);

    return 0;
}

int fpga__passiveserialconfigure(const uint8_t *data, unsigned len)
{
    uint8_t txrxbuf[128];

    ESP_LOGI(TAG,"Loading FPGA bitfile (%d bytes)", len);


    gpio_set_level( PIN_NUM_NCONFIG, 1 );
    gpio_set_level( PIN_NUM_NCONFIG, 0 );
    vTaskDelay(1 / portTICK_RATE_MS);
    gpio_set_level( PIN_NUM_NCONFIG, 1 );

    if (gpio__waitpin( PIN_NUM_NSTATUS, 1, 100)<0) {
        ESP_LOGW(TAG,"FPGA pin NSTATUS failed to go HIGH!");
        return -1;
    }

    while (len) {
        int chunk = MIN(len,sizeof(txrxbuf));
        int i;
        for (i=0;i<chunk;i++) {
            txrxbuf[i] = bitrev__byte(*data++);
        }
        spi__transceive(spi0_fpga, txrxbuf, chunk);
        if (gpio_get_level(PIN_NUM_NSTATUS)==0) {
            ESP_LOGW(TAG,"FPGA pin NSTATUS LOW while uploading (%d remaining)", len);
            return -1;
        }
        len-=chunk;
    }

    while(1) {
        if (gpio__waitpin( PIN_NUM_CONF_DONE, 1, 1000)<0) {
            ESP_LOGW(TAG,"FPGA pin CONF_DONE failed to go LOW!");
        } else {
            break;
        }
        ESP_LOGI(TAG, "CONF_DONE: %d", gpio_get_level(PIN_NUM_CONF_DONE) );
    }

    return 0;
}

int fpga__passiveserialconfigure_fromfile(int fh, unsigned len)
{
    uint8_t txrxbuf[128];

    ESP_LOGI(TAG,"Loading FPGA bitfile (%d bytes)", len);


    gpio_set_level( PIN_NUM_NCONFIG, 1 );
    gpio_set_level( PIN_NUM_NCONFIG, 0 );
    vTaskDelay(1 / portTICK_RATE_MS);
    gpio_set_level( PIN_NUM_NCONFIG, 1 );

    if (gpio__waitpin( PIN_NUM_NSTATUS, 1, 100)<0) {
        ESP_LOGW(TAG,"FPGA pin NSTATUS failed to go HIGH!");
        return -1;
    }

    while (len) {
        int chunk = MIN(len,sizeof(txrxbuf));
        int i;
        // Read first.
        int r = read(fh, txrxbuf, chunk);
        if (r!=chunk) {
            ESP_LOGE(TAG,"Short read from file!");
            return -1;
        }
        for (i=0;i<chunk;i++) {
            txrxbuf[i] = bitRevTable[ txrxbuf[i] ];
        }
        spi__transceive(spi0_fpga, txrxbuf, chunk);
        if (gpio_get_level(PIN_NUM_NSTATUS)==0) {
            ESP_LOGW(TAG,"FPGA pin NSTATUS LOW while uploading (%d remaining)", len);
            return -1;
        }
        len-=chunk;
    }

    while(1) {
        if (gpio__waitpin( PIN_NUM_CONF_DONE, 1, 1000)<0) {
            ESP_LOGW(TAG,"FPGA pin CONF_DONE failed to go LOW!");
        } else {
            break;
        }
        ESP_LOGI(TAG, "CONF_DONE: %d", gpio_get_level(PIN_NUM_CONF_DONE) );
    }

    return 0;
}



int fpga__read_command_fifo(uint8_t *dest)
{
    int r = fpga__get_status();
    if (r<0)
        return r;

    unsigned used = FPGA_STATUS_CMDFIFO_USED(r);

    if (used==0)
        return -1;


    r = fpga__issue_read_block(FPGA_SPI_CMD_READ_CMDFIFO, dest, used);

    if (r<0)
        return r;

    return used;
}

static uint16_t fpga__get_tap_fifo_usage()
{
    uint8_t buf[2];
    ESP_ERROR_CHECK(fpga__issue_read(FPGA_SPI_CMD_READ_TAPFIFO_USAGE, buf, sizeof(buf)));

    return extractbe16(&buf[0]);
}


uint16_t fpga__get_tap_fifo_free()
{
    uint16_t used = fpga__get_tap_fifo_usage();
    if (used&0x8000)
        return 0;
    return FPGA_TAP_FIFO_SIZE - used;
}

bool fpga__tap_fifo_empty()
{
    return fpga__get_tap_fifo_usage() == 0;
}

int fpga__load_tap_fifo(const uint8_t *data, unsigned len, int timeout)
{
#define TAP_LOCAL_CHUNK_SIZE 256

    uint16_t stat = fpga__get_tap_fifo_usage();
//    ESP_LOGI(TAG, "TAP stat %04x\n", stat);

    if (stat& 0x8000)
        return 0; // FIFO full

    uint16_t maxsize = FPGA_TAP_FIFO_SIZE - stat; // Get available size.


    if (maxsize > TAP_LOCAL_CHUNK_SIZE)
        maxsize = TAP_LOCAL_CHUNK_SIZE;

    if (maxsize>len)
        maxsize = len;

    // Upload chunk
    if (maxsize>0) {
        if (fpga__issue_write(FPGA_SPI_CMD_WRITE_TAPFIFO, data, maxsize)<0)
            return -1;
    }

    return maxsize;
}

int fpga__load_tap_fifo_command(const uint8_t *data, unsigned len, int timeout)
{
#define TAP_LOCAL_CHUNK_SIZE 256

    uint16_t stat = fpga__get_tap_fifo_usage();
    //ESP_LOGI(TAG, "TAP stat %04x\n", stat);

    if (stat& 0x8000)
        return 0; // FIFO full

    uint16_t maxsize = FPGA_TAP_FIFO_SIZE - stat; // Get available size.


    if (maxsize > TAP_LOCAL_CHUNK_SIZE)
        maxsize = TAP_LOCAL_CHUNK_SIZE;

    if (maxsize>len)
        maxsize = len;

    // Upload chunk
    if (maxsize>0) {
        if (fpga__issue_write(FPGA_SPI_CMD_WRITE_TAPCMD, data, maxsize)<0)
            return -1;
    }

    return maxsize;
}


int fpga__read_extram(uint32_t address)
{
#if 0
    uint8_t buf[6];
    buf[0] = 0x50;
    buf[1] = (address>>16) & 0xff;
    buf[2] = (address>>8) & 0xff;
    buf[3] = (address) & 0xff;
    buf[4] = 0x00; // Dummy
    buf[5] = 0x00;

    if (spi__transceive(spi0_fpga_10m, buf, 6)<0)
        return -1;

    return buf[5];
#else
    uint8_t buf[1];

    if (fpga__issue_read_addr24(FPGA_SPI_CMD_READ_EXTRAM, address, buf, 1)<0)
        return -1;

    return buf[0];

#endif
}

int fpga__write_extram(uint32_t address, uint8_t val)
{
#if 0
    uint8_t buf[6];
    buf[0] = 0x51;
    buf[1] = (address>>16) & 0xff;
    buf[2] = (address>>8) & 0xff;
    buf[3] = (address) & 0xff;
    buf[4] = val;

    if (spi__transceive(spi0_fpga, buf, 5)<0)
        return -1;
    return 0;
#else
    return  spi__transceive_cmd8_addr24(spi0_fpga,
                                        0x51,
                                        address,
                                        &val, 1);
#endif
}

int fpga__read_extram_block(uint32_t address, uint8_t *dest, int size)
{
    return spi__transceive_cmd8_addr32(spi0_fpga,
                                       0x50,
                                       address<<8,
                                       dest, size);
}

int fpga__write_extram_block(uint32_t address, const uint8_t *buffer, int size)
{
    //ESP_LOGI(TAG, "Write RAM address 0x%06x len %d", address, size);

    return fpga__issue_write_addr24(FPGA_SPI_CMD_WRITE_EXTRAM,
                                    address,
                                    buffer,
                                    size);
}


int fpga__read_usb(uint16_t address)
{
    uint8_t v;
    int r = fpga__read_usb_block(address, &v, 1);
    if (r<0)
        return r;
    return v;

}

int fpga__read_usb_block(uint16_t address, uint8_t *dest, int size)
{
    return fpga__issue_read_addr16(FPGA_SPI_CMD_READ_USB,
                                   address,
                                   dest,
                                   size);
}

int fpga__write_usb(uint16_t address, uint8_t val)
{
    uint8_t v = val;
    return fpga__write_usb_block(address, &v, 1);
}

int fpga__write_usb_block(uint16_t address, const uint8_t *buffer, int size)
{
    return fpga__issue_write_addr16(FPGA_SPI_CMD_WRITE_USB,
                                   address,
                                   buffer,
                                   size);
}

void fpga__set_config1_bits(uint32_t bits)
{
    config1_latch |= bits;
    fpga__set_register(REG_CONFIG1, config1_latch);
}

void fpga__clear_config1_bits(uint32_t bits)
{
    config1_latch &= ~bits;
    fpga__set_register(REG_CONFIG1, config1_latch);
}

static int fpga__set_romram(uint8_t romram)
{
    return fpga__issue_write(FPGA_SPI_CMD_SET_ROMRAM, &romram, 1);
}

int fpga__set_ram(uint8_t ram)
{
    // Lower 3 bits set ram number, MSB sets RAM
    return fpga__set_romram(((ram & 0x7) | 0x80));
}

int fpga__set_rom(uint8_t rom)
{
    // Lower 2 bits set ROM number, MSB cleared.
    // Note that NMI ROM is always ROM 0.
    return fpga__set_romram(rom & 0x3);
}

int fpga__write_extram_block_from_file(uint32_t address, int fd, int size, bool verify)
{
    uint8_t chunk[128];

    while (size) {
        int chunksize = MIN(size, sizeof(chunk));

        int r = read(fd, chunk, chunksize);
        if (r!=chunksize) {
            ESP_LOGE(TAG, "Short read from file: %s", strerror(errno));
            close(fd);
            return -1;
        }
        if (fpga__write_extram_block(address, chunk, chunksize)<0) {
            ESP_LOGE(TAG, "Error writing address 0x%06x", address);
            close(fd);
            return -1;
        }
        address += chunksize;
        size -= chunksize;
    }
    return 0;
}

int fpga__isBITmode(void)
{
    int f = fpga__get_status();
    return f & FPGA_STATUS_BITMODE_REQUESTED;
}

int fpga__read_uart_status(void)
{
    uint8_t stat;
    if (fpga__issue_read(FPGA_SPI_CMD_READ_UART_STATUS, &stat, 1)<0)
        return -1;
    return stat;

}
int fpga__read_uart_data(uint8_t *buf, int len)
{
    return fpga__issue_read_block(FPGA_SPI_CMD_READ_UART_DATA, buf, len);
}

int fpga__write_uart_data(uint8_t v)
{
    return fpga__issue_write(FPGA_SPI_CMD_WRITE_UART_DATA, &v, 1);
}

int fpga__write_bit_data(const uint8_t *data, unsigned len)
{
    return fpga__issue_write(FPGA_SPI_CMD_WRITE_BIT, data, len);
}

int fpga__read_bit_data(uint8_t *buf, unsigned len)
{
    return fpga__issue_read(FPGA_SPI_CMD_READ_BIT, buf, len);

}

int fpga__read_capture_block(uint16_t address, uint8_t *dest, int size)
{
    return fpga__issue_read_addr16(FPGA_SPI_CMD_READ_CAP,
                                   address,
                                   dest,
                                   size);
}

int fpga__write_capture_block(uint16_t address, const uint8_t *buffer, int size)
{
    return fpga__issue_write_addr16(FPGA_SPI_CMD_WRITE_CAP,
                                   address,
                                   buffer,
                                   size);
}

int fpga__disable_hooks(void)
{
    uint8_t buf[1];
    buf[0] = 0;
    unsigned i;
    unsigned address = 0x43;
    for (i=0;i<4;i++) {
        fpga__issue_write_addr16(FPGA_SPI_CMD_WRITE_CTRL,
                                 address,
                                 &buf[0],
                                 1);
        address+=0x04;
    }
    return 0;
}

int fpga__enable_hook(uint8_t index, uint16_t start, uint8_t len)
{
    uint8_t buf[4];
    buf[0] = start & 0xff;
    buf[1] = start >> 8;
    buf[2] = len-1;
    buf[3] = 1;

    return fpga__issue_write_addr16(FPGA_SPI_CMD_WRITE_CTRL,
                                    0x40 + (index*0x04),
                                    &buf[0],
                                    4);
}

int fpga__readinterrupt(void)
{
    uint8_t intstat;

    int r = fpga__issue_read(FPGA_SPI_CMD_READ_INTERRUPT_STATUS, &intstat, 1);
    if (r<0)
        return -1;
    return intstat;

}
int fpga__ackinterrupt(uint8_t mask)
{
    return fpga__issue_write(FPGA_SPI_CMD_READ_INTERRUPT_ACK, &mask, 1);
}
