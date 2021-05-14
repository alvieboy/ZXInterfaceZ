/**
 * \defgroup fpga FPGA communication routines
 * \brief FPGA communication routines
 *
 * These functions should be used to communicate with the FPGA.
 *
 * Refer to the FPGA architecture document for details
 *
 */

#include "fpga.h"
#include "spi.h"
#include "defs.h"
#include "dump.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "os/semaphore.h"
#include "os/core.h"
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
#include "stream.h"
#include "flash_resource.h"

#define TAG "FPGA"

#define USE_DUAL_SPI_CS

#ifdef USE_DUAL_SPI_CS

static spi_device_handle_t spi0_fpga;
static spi_device_handle_t spi0_fpga_slow;
#else
static spi_device_handle_t spi0_fpga;
#define spi0_fpga_slow spi0_fpga
#endif

// Do we need a mutex here?

static Semaphore fpga_lock;

static volatile fpga_flags_t latched_flags = 0;
static volatile uint32_t config1_latch = 0;


static uint32_t fpga_id;

static void fpga__get_binary_filename(char *target)
{
    sprintf(target, "%s/fpga0.rbf", flash_resource__get_root());
}


/**
 \ingroup fpga
 \brief Return the FPGA ID.
 \return uint32_t representing the FPGA identification
 */
uint32_t fpga__id(void)
{
    return fpga_id;
}

static void fpga__init_spi()
{
#ifdef USE_DUAL_SPI_CS
    spi__init_device(&spi0_fpga,      20000000, PIN_NUM_CS);  // 40MHz
#endif
    spi__init_device(&spi0_fpga_slow, 10000000, PIN_NUM_CS2); // 10Mhz, mostly for RAM access
}

static int fpga__issue_read(uint8_t cmd, spi_transceive_buffer_t buf, unsigned size)
{
    return spi__transceive_cmd8_addr8(spi0_fpga, cmd, 0xFF, buf, size);
}

static int fpga__issue_read_addr8(uint8_t cmd, uint8_t addr, spi_transceive_buffer_t buf, unsigned size)
{
    return spi__transceive_cmd8_addr16(spi0_fpga, cmd, (uint16_t)addr<<8, buf, size);
}

static int fpga__issue_read_block(uint8_t cmd, spi_transceive_buffer_t buf, uint8_t size)
{
    // reuse addr16 as length indicator.
    uint8_t addr = size;
    return fpga__issue_read_addr8(cmd, addr, buf, size);
}

static int fpga__issue_read_addr16(uint8_t cmd, uint16_t addr, spi_transceive_buffer_t buf, unsigned size)
{
    return spi__transceive_cmd8_addr24(spi0_fpga, cmd, (uint32_t)addr<<8, buf, size);
}
#if 0
static int fpga__issue_read_addr24(uint8_t cmd, uint32_t addr, spi_transceive_buffer_t buf, unsigned size)
{
    return spi__transceive_cmd8_addr32(spi0_fpga, cmd, (uint32_t)addr<<8, buf, size);
}
#endif

static int fpga__issue_read_addr24_slow(uint8_t cmd, uint32_t addr, spi_transceive_buffer_t buf, unsigned size)
{
    return spi__transceive_cmd8_addr32(spi0_fpga_slow, cmd, (uint32_t)addr<<8, buf, size);
}

static int fpga__issue_write(uint8_t cmd, spi_transmit_buffer_t buf, unsigned size)
{
    return spi__transmit_cmd8(spi0_fpga, cmd, buf, size);
}

#if 0
static int fpga__issue_write_addr24(uint8_t cmd, uint32_t address, spi_transmit_buffer_t buf, unsigned size)
{
    return spi__transmit_cmd8_addr24(spi0_fpga, cmd, address, buf, size);
}
#endif

static int fpga__issue_write_addr24_slow(uint8_t cmd, uint32_t address, spi_transmit_buffer_t buf, unsigned size)
{
    return spi__transmit_cmd8_addr24(spi0_fpga_slow, cmd, address, buf, size);
}

static int fpga__issue_write_addr16(uint8_t cmd, uint16_t address, spi_transmit_buffer_t buf, unsigned size)
{
    return spi__transmit_cmd8_addr16(spi0_fpga, cmd, address, buf, size);
}

static unsigned fpga__read_id()
{
    //uint8_t idbuf[4] = {0};
    union u32 idbuf = {0};
    int r = fpga__issue_read(FPGA_SPI_CMD_READ_ID, &idbuf.w32, 4);

    if (r<0) {
        ESP_LOGE(TAG, "SPI transceive: error %d", r);
        return 0;
    }
    printf("FPGA id: ");
    dump__buffer(&idbuf.w8[0], 4);
    printf("\r\n");

    return extractbe32(&idbuf.w8[0]);
}

static int fpga__configurefromflash()
{
#ifndef __linux__
    char filename[128];
    int fh;
    int r = -1;
    struct stat st;
    fpga__get_binary_filename(filename);
    do {
        r = __lstat(filename, &st);
        if (r<0) {
            ESP_LOGE(TAG, "Cannot stat FPGA image");
            break;
        }
        fh = __open(filename, O_RDONLY);
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

/**
 \ingroup fpga
 \brief Initialise the FPGA

 This routine will initialise the FPGA, loading the FPGA bitfile from the internal SPIFFS

 \return 0 if successful
 */

int fpga__init()
{
    fpga_lock = semaphore__create_mutex();

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

/**
 \ingroup fpga
 \brief Read status register from the FPGA

 If unsucessful, the system will panic.

 \return The status register value.
 */
uint8_t fpga__get_status()
{
    union u32 buf;

    ESP_ERROR_CHECK(fpga__issue_read(FPGA_SPI_CMD_READ_STATUS, &buf.w32, 1));

    return buf.w8[0];
}

/**
 \ingroup fpga
 \brief Return the last known Spectrum PC (Program Counter)

 If unsuccessful, the system will panic.

 \return The last known PC
 */
uint16_t fpga__get_spectrum_pc()
{
    union u32 buf;
    ESP_ERROR_CHECK(fpga__issue_read(FPGA_SPI_CMD_READ_PC, &buf.w32, 2));
    return extractbe16(&buf.w8[0]);
}

/**
 \ingroup fpga
 \brief Set and/or clear FPGA flags

 This routine disables all flags depicted in the disable parameter,
 and then sets all flags depicted in the enable parameter.

 If unsuccessful, the system will panic.

 \param enable Flags to be enabled
 \param disable Flags to be disabled
 */
void fpga__set_clear_flags(fpga_flags_t enable, fpga_flags_t disable)
{
    uint8_t buf[3];

    semaphore__take(fpga_lock, OS_MAX_DELAY);
    fpga_flags_t newflags = (latched_flags & ~disable) | enable;

    buf[0] = newflags&0xff;
    buf[1] = 0x00;
    buf[2] = newflags>>8;

    latched_flags = newflags;
    semaphore__give(fpga_lock);

    ESP_LOGI(TAG,"Set flags 0x%08x enable 0x%08x disable 0x%08x", newflags, enable, disable);


    ESP_ERROR_CHECK(fpga__issue_write(FPGA_SPI_CMD_WRITE_FLAGS, buf, sizeof(buf)));

}

/**
 \ingroup fpga
 \brief Set a trigger on the FPGA

 This routine sets one or more "triggers" on the FPGA. See FPGA_FLAG_TRIG* macros.

 If unsuccessful, the system will panic.

 \param enable Flags to be enabled
 \param disable Flags to be disabled
 */
void fpga__set_trigger(uint8_t trig)
{
    uint8_t buf[3];

    semaphore__take(fpga_lock, OS_MAX_DELAY);

    buf[0] = latched_flags & 0xff;
    buf[1] = trig;
    buf[2] = latched_flags >> 8;

    semaphore__give(fpga_lock);

    ESP_ERROR_CHECK(fpga__issue_write(FPGA_SPI_CMD_WRITE_FLAGS, buf, sizeof(buf)));
}

/**
 \ingroup fpga
 \brief Get the current ZX Spectrum framebuffer

 Get (extract) the ZX Spectrum framebuffer contents into the specified memory area.

 If unsuccessful, the system will panic.

 \param target Where to store the framebuffer ()
 */
void fpga__get_framebuffer(uint32_t *target)
{
    uint8_t dummy[1] = { 0x00 };
    ESP_ERROR_CHECK(fpga__issue_read_addr16(FPGA_SPI_CMD_READ_VIDEOMEM, 0x0000, target, SPECTRUM_FRAME_SIZE));
    // Notify frame grabbed.
    ESP_ERROR_CHECK(fpga__issue_write(FPGA_SPI_CMD_SETEOF, dummy, sizeof(dummy)));
}

/**
 \ingroup fpga
 \brief Set the value of an FPGA REG32 register

 See REG_ macros for a description of the registers, or refer to the FPGA architecture document.

 If unsuccessful, the system will panic.

 \param reg the register number to set
 \param value value to be placed on register
 */
void fpga__set_register(uint8_t reg, uint32_t value)
{
    uint8_t buf[5];
    buf[0] = reg;
    buf[1] = (value >> 24)& 0xff;
    buf[2] = (value >> 16)& 0xff;
    buf[3] = (value >> 8)& 0xff;
    buf[4] = (value)& 0xff;
    ESP_LOGI(TAG,"Writing register %d\n", reg);
    ESP_ERROR_CHECK(fpga__issue_write(FPGA_SPI_CMD_WRITE_REG32, buf, sizeof(buf)));
}
 /**
 \ingroup fpga
 \brief Get the value of an FPGA REG32 register

 See REG_ macros for a description of the registers, or refer to the FPGA architecture document.

 If unsuccessful, the system will panic.

 \param reg the register number to get
 \return the value of the register
 */
uint32_t fpga__get_register(uint8_t reg)
{
    union u32 buf;

    ESP_ERROR_CHECK(fpga__issue_read_addr8(FPGA_SPI_CMD_READ_REG32, reg, &buf.w32, sizeof(buf)));

    return extractbe32(&buf.w8[0]);
}

/**
 * \ingroup fpga
 * \defgroup fpgarom ROM routines
 * \brief FPGA ROM related routines
 *
 * These routines are used to perform ROM related operations
 */

/**
 * \ingroup fpgarom
 * \brief Upload a ROM chunk into a specified external RAM address
 * \deprecated This function is deprecated.
 */
int fpga__upload_rom_chunk(uint32_t baseaddress, uint16_t offset, uint8_t *buffer_sub3, unsigned len)
{
    if (fpga__write_extram_block(baseaddress + (uint32_t)offset, &buffer_sub3[3], len)<0) {
        ESP_LOGE(TAG, "Cannot write ROM block");
        return -1;
    }
    return len;
}

/**
 * \ingroup fpga
 * \defgroup fpgafifo FPGA FIFO routines
 * \brief Control of the FPGA FIFO
 */

/**
 * \ingroup fpgafifo
 * \brief Load resource FIFO
 *
 * Loads the resource FIFO with the specified amount of data.
 *
 * \param data The data to load
 * \param len Size of data, in bytes
 * \param timeout timeout, in hundreds of milisseconds
 * \return 0 if successful
 */
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

/**
 * \ingroup fpgarom
 * \brief Write on the current ROM
 *
 * \deprecated This function is deprecated.
 */
int fpga__write_rom(unsigned offset, uint8_t val)
{
    ESP_LOGI(TAG,"Patching rom @ 0x%08x: 0x%02x\n", offset, val);
    return fpga__write_extram(offset, val);
}

/**
 * \ingroup fpgarom
 * \brief Upload a ROM to a specific address
 * \param baseaddress Address to load ROM
 * \param buffer the ROM data
 * \param len lenght in bytes
 * \return 0 if successful
 */

int fpga__upload_rom(uint32_t baseaddress, const uint8_t *buffer, unsigned len)
{
    uint16_t offset = 0;
#ifdef __linux
    union {
        uint32_t w32[1024];
        uint8_t w8[4096];
    } tbuf;
#else
    union {
        uint32_t w32[16];
        uint8_t w8[64];
    } tbuf;
#endif
    ESP_LOGI(TAG, "Uploading ROM, %d bytes", len);
    do {
        int llen = MIN(len,sizeof(tbuf));

        if (fpga__write_extram_block(baseaddress+(uint32_t)offset, buffer, llen)<0)
            return -1;
#if 1
        if (fpga__read_extram_block(baseaddress+(uint32_t)offset, &tbuf.w32[0], llen)<0)
            return -1;

        if (memcmp(&tbuf.w8[0], buffer, llen)!=0) {
            ESP_LOGE(TAG,"ERROR comparing ROM contents\n");
            dump__buffer(buffer, llen);
            dump__buffer(&tbuf.w8[0], llen);

            memset(&tbuf.w8[0], 0, sizeof(tbuf));

            if (fpga__read_extram_block(baseaddress+(uint32_t)offset, &tbuf.w32[0], llen)<0)
                return -1;

            dump__buffer(&tbuf.w8[0], llen);

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

/**
 * \ingroup fpga
 * \defgroup fpgaconf FPGA configuration
 * \brief Configuration of FPGA
 */


/**
 * \ingroup fpgaconf
 * \brief Load FPGA bitfile from memory
 */
int fpga__passiveserialconfigure(const uint8_t *data, unsigned len)
{
    union {
        uint32_t w32[32];
        uint32_t w8[128];
    } txrxbuf;

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
            txrxbuf.w8[i] = bitrev__byte(*data++);
        }
        spi__transceive(spi0_fpga, &txrxbuf.w32[0], chunk);
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

/**
 * \ingroup fpgaconf
 * \brief Load FPGA bitfile from file
 */
int fpga__passiveserialconfigure_fromfile(int fh, unsigned len)
{
    union {
        uint32_t w32[32];
        uint8_t w8[128];
    } txrxbuf;

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
        int r = read(fh, &txrxbuf.w8[0], chunk);
        if (r!=chunk) {
            ESP_LOGE(TAG,"Short read from file!");
            return -1;
        }
        for (i=0;i<chunk;i++) {
            txrxbuf.w8[i] = bitRevTable[ txrxbuf.w8[i] ];
        }
        spi__transceive(spi0_fpga, &txrxbuf.w32[0], chunk);
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


/**
 * \ingroup fpgafifo
 * \brief Fetch data from FPGA commmand fifo
 *
 * Maximum size is 4 bytes, so ensure buffer has 4 bytes available.
 * The buffer is required to be 32-bit aligned and a multiple of 32-bits in size
 *
 */
int fpga__read_command_fifo(uint32_t *dest)
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

/**
 * \ingroup fpgafifo
 * \brief Get the current TAP fifo usage
 */
static uint16_t fpga__get_tap_fifo_usage()
{
    union u32 buf;
    ESP_ERROR_CHECK(fpga__issue_read(FPGA_SPI_CMD_READ_TAPFIFO_USAGE, &buf.w32, sizeof(buf)));

    return extractbe16(&buf.w8[0]);
}


/**
 * \ingroup fpgafifo
 * \brief Get the current TAP fifo free size (in bytes)
 */
uint16_t fpga__get_tap_fifo_free()
{
    uint16_t used = fpga__get_tap_fifo_usage();
    if (used&0x8000)
        return 0;
    return FPGA_TAP_FIFO_SIZE - used;
}

/**
 * \ingroup fpgafifo
 * \brief Check if TAP fifo is empty
 */
bool fpga__tap_fifo_empty()
{
    return fpga__get_tap_fifo_usage() == 0;
}

/**
 * \ingroup fpgafifo
 * \brief load data into the TAP fifo (data mode)
 */

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

/**
 * \ingroup fpgafifo
 * \brief load data into the TAP fifo (command mode)
 */
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

/**
 * \ingroup fpga
 * \defgroup fpgaram FPGA external RAM manipulation
 * \brief Reading and writing from/to external RAM
 */

/**
 * \ingroup fpgaram
 * \brief Read a single byte from external RAM
 */
int fpga__read_extram(uint32_t address)
{
    union u32 buf;

    if (fpga__issue_read_addr24_slow(FPGA_SPI_CMD_READ_EXTRAM, address, &buf.w32, 1)<0)
        return -1;

    return buf.w8[0];
}


/**
 * \ingroup fpgaram
 * \brief Write a single byte from external RAM
 */
int fpga__write_extram(uint32_t address, uint8_t val)
{
    return spi__transmit_cmd8_addr24(spi0_fpga_slow,
                                     FPGA_SPI_CMD_WRITE_EXTRAM,
                                     address,
                                     &val, 1);
}

/**
 * \ingroup fpgaram
 * \brief Read a block of data from external RAM.
 *
 * The buffer is required to be 32-bit aligned and a multiple of 32-bits in size.
 *
 * \param address external RAM address to start read from
 * \param dest buffer to hold data
 * \param size size, in bytes, to be read
 * \return 0 if successful
 */

int fpga__read_extram_block(uint32_t address, uint32_t *dest, int size)
{
    return spi__transceive_cmd8_addr32(spi0_fpga_slow,
                                       FPGA_SPI_CMD_READ_EXTRAM,
                                       address<<8,
                                       dest, size);
}


/**
 * \ingroup fpgaram
 * \brief Write a block of data to external RAM.
 * \param address external RAM address to start writing to
 * \param buffer Data to be written
 * \param size size, in bytes, to be written
 * \return 0 if successful
 */
int fpga__write_extram_block(uint32_t address, const uint8_t *buffer, int size)
{
    return fpga__issue_write_addr24_slow(FPGA_SPI_CMD_WRITE_EXTRAM,
                                         address,
                                         buffer,
                                         size);
}


/**
 * \ingroup fpga
 * \defgroup fpgausb FPGA USB interaction
 * \brief USB interaction
 *
 * See FPGA architecture document for USB memory map.
 */


/**
 * \ingroup fpgausb
 * \brief Read a single byte from USB address space
 */
int fpga__read_usb(uint16_t address)
{
    union u32 v;
    int r = fpga__read_usb_block(address, &v.w32, 1);
    if (r<0)
        return r;
    return v.w8[0];

}

/**
 * \ingroup fpgausb
 * \brief Read block of data from USB address space
 *
 * The buffer is required to be 32-bit aligned and a multiple of 32-bits in size.
 */
int fpga__read_usb_block(uint16_t address, uint32_t *dest, int size)
{
    return fpga__issue_read_addr16(FPGA_SPI_CMD_READ_USB,
                                   address,
                                   dest,
                                   size);
}

/**
 * \ingroup fpgausb
 * \brief Write a single byte to USB address space
 */
int fpga__write_usb(uint16_t address, uint8_t val)
{
    uint8_t v = val;
    return fpga__write_usb_block(address, &v, 1);
}

/**
 * \ingroup fpgausb
 * \brief Write a block of data to USB address space
 */
int fpga__write_usb_block(uint16_t address, const uint8_t *buffer, int size)
{
    return fpga__issue_write_addr16(FPGA_SPI_CMD_WRITE_USB,
                                   address,
                                   buffer,
                                   size);
}

/**
 * \ingroup fpga
 * \brief Set CONFIG1 bits
 *
 * Sets the provided bits in the CONFIG1 register.
 *
 */

void fpga__set_config1_bits(uint32_t bits)
{
    semaphore__take(fpga_lock, OS_MAX_DELAY);
    config1_latch |= bits;
    semaphore__give(fpga_lock);
    //ESP_LOGI(TAG,"Setting config1 bits 0x%08x", bits);
    fpga__set_register(REG_CONFIG1, config1_latch);
}

/**
 * \ingroup fpga
 * \brief Clear CONFIG1 bits
 *
 * Clears the provided bits in the CONFIG1 register.
 *
 */
void fpga__clear_config1_bits(uint32_t bits)
{
    semaphore__take(fpga_lock, OS_MAX_DELAY);
    config1_latch &= ~bits;
    semaphore__give(fpga_lock);
    //ESP_LOGI(TAG,"Clear config1 bits 0x%08x", bits);
    fpga__set_register(REG_CONFIG1, config1_latch);
}

static int fpga__set_romram(uint8_t romram)
{
    return fpga__issue_write(FPGA_SPI_CMD_SET_ROMRAM, &romram, 1);
}

/**
 * \ingroup fpga
 * \brief Set current RAM
 *
 * Sets the current RAM available to Spectrum. There are 8 RAMs available.
 *
 * \param ram RAM to be made available in 0x2000->0x3FFF Z80 address space (0 to 7)
 *
 */
int fpga__set_ram(uint8_t ram)
{
    // Lower 3 bits set ram number, MSB sets RAM
    return fpga__set_romram(((ram & 0x7) | 0x80));
}

/**
 * \ingroup fpga
 * \brief Set current ROM
 *
 * Sets the current ROM available to Spectrum. There are 4 ROMs available.
 *
 * The first two ROMS (0 and 1)are 4KB in size.
 * The third ROM (2) is 8KB in size.
 * The fourth ROM (3) is 32KB in size.
 *
 * \param ram ROM to be made available in Z80 address space (0 to 3)
 *
 */
int fpga__set_rom(uint8_t rom)
{
    // Lower 2 bits set ROM number, MSB cleared.
    // Note that NMI ROM is always ROM 0.
    return fpga__set_romram(rom & 0x3);
}

/**
 * \ingroup fpgaram
 * \brief Write a block of data to external RAM from a file.
 *
 * This method waits until all data has been written.
 *
 * \param address external RAM address to start writing to
 * \param fd File descriptor
 * \param size size, in bytes, to be written
 * \return 0 if successful
 */
int fpga__write_extram_block_from_file(uint32_t address, int fd, int size)
{
    uint8_t chunk[128];

    while (size) {
        int chunksize = MIN(size, (int)sizeof(chunk));

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

/**
 * \ingroup fpgaram
 * \brief Write a block of data to external RAM from a stream.
 *
 * This method waits until all data has been written.
 *
 * \param address external RAM address to start writing to
 * \param stream Stream
 * \param size size, in bytes, to be written
 * \return 0 if successful
 */
int fpga__write_extram_block_from_stream(uint32_t address, struct stream *stream, int size)
{
#ifdef __linux__
    uint8_t chunk[4096];
#else
    uint8_t chunk[128];
#endif

    while (size) {
        int chunksize = MIN(size, (int)sizeof(chunk));

        int r = stream__read(stream, chunk, chunksize);
        if (r<=0) {
            ESP_LOGE(TAG, "Short read from file: %s", strerror(errno));
            return -1;
        }
        if (fpga__write_extram_block(address, chunk, r)<0) {
            ESP_LOGE(TAG, "Error writing address 0x%06x", address);
            return -1;
        }
        address += r;
        size -= r;
    }
    return 0;
}


/**
 * \ingroup fpgaram
 * \brief Write a block of data to external RAM from a file, giving up if there is an error
 * or if data is not available
 *
 * This method does not wait for all data to be written. 
 *
 * \param address external RAM address to start writing to
 * \param fd File descriptor
 * \param size size, in bytes, to be written
 * \param writtensize pointer to size, in bytes, that was successfully written.
 * \return 0 if successful
 */
int fpga__write_extram_block_from_file_nonblock(uint32_t address, int fd, int size,
                                                int *writtensize)
{
    uint8_t chunk[128];
    *writtensize = 0;

    while (size) {
        int chunksize = MIN(size, (int)sizeof(chunk));

        int r = read(fd, chunk, chunksize);
        if (r<0) {
            ESP_LOGE(TAG, "Short read from file: %s", strerror(errno));
            close(fd);
            return -1;
        }
        if (r==0) {
            return 0;
        }
        if (fpga__write_extram_block(address, chunk, r)<0) {
            ESP_LOGE(TAG, "Error writing address 0x%06x", address);
            close(fd);
            return -1;
        }
        address += r;
        size -= r;
        *writtensize += r;
    }
    return 0;
}

/**
 * \ingroup fpgaram
 * \brief Read a block of data from external RAM into a file.
 *
 * If checksum is not NULL, the checksum of the data written will be placed in the location.
 *
 * \param address external RAM address to start read from
 * \param fd file descriptor of file to be written
 * \param size size, in bytes, to be read
 * \param checksum pointer to checksum which will be filled in with the data checksum.
 * \return 0 if successful
 */
int fpga__read_extram_block_into_file(uint32_t address, int fd, int size, uint8_t *checksum)
{
    union {
        uint32_t w32[32];
        uint8_t w8[128];
    } chunk;

    while (size) {
        int chunksize = MIN(size, (int)sizeof(chunk));
        int r = fpga__read_extram_block(address, &chunk.w32[0], chunksize);
        if (r<0)
            return -1;
        if (checksum) {
            uint8_t lchecksum = 0;
            for (int i=0; i<chunksize; i++) {
                lchecksum ^= chunk.w8[i];
            }
            (*checksum) ^= lchecksum;
        }
        r = write(fd, &chunk.w8[0], chunksize);
        if (r!=chunksize) {
            ESP_LOGE(TAG, "Short write into file: %s", strerror(errno));
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
    union u32 stat;
    if (fpga__issue_read(FPGA_SPI_CMD_READ_UART_STATUS, &stat.w32, 1)<0)
        return -1;
    return stat.w8[0];

}
int fpga__read_uart_data(uint32_t *buf, int len)
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

int fpga__read_bit_data(uint32_t *buf, unsigned len)
{
    return fpga__issue_read(FPGA_SPI_CMD_READ_BIT, buf, len);

}

/**
 * \ingroup fpga
 * \defgroup fpgacap Data capture
 * \brief Data capture routines
 *
 * See FPGA architecture document for details.
 */



/**
 * \ingroup fpgacap
 * \brief Read capture data from the capture address space
 */
int fpga__read_capture_block(uint16_t address, uint32_t *dest, int size)
{
    return fpga__issue_read_addr16(FPGA_SPI_CMD_READ_CAP,
                                   address,
                                   dest,
                                   size);
}

/**
 * \ingroup fpgacap
 * \brief Write capture data to the capture address space
 *
 * The buffer is required to be 32-bit aligned and a multiple of 32-bits in size.
 *
 */
int fpga__write_capture_block(uint16_t address, const uint8_t *buffer, int size)
{
    return fpga__issue_write_addr16(FPGA_SPI_CMD_WRITE_CAP,
                                   address,
                                   buffer,
                                   size);
}
/**
 * \ingroup fpga
 * \defgroup fpgahook ROM hook manipulation
 * \brief ROM hooks
 *
 * See FPGA architecture document for details.
 */


/**
 * \ingroup fpgahook
 * \brief Disable a ROM hook
 */
int fpga__disable_hook(uint8_t index)
{
    uint8_t flags = 0;
    return fpga__issue_write_addr16(FPGA_SPI_CMD_WRITE_CTRL,
                                    0x43 + (index*0x04),
                                    &flags,
                                    1);
}

/**
 * \ingroup fpgahook
 * \brief Write (configure) a ROM hook
 */
int fpga__write_hook(uint8_t index, uint16_t start, uint8_t len, uint8_t flag)
{
    uint8_t buf[4];
    buf[0] = start & 0xff;
    buf[1] = start >> 8;
    buf[2] = len;
    buf[3] = flag;

    return fpga__issue_write_addr16(FPGA_SPI_CMD_WRITE_CTRL,
                                    0x40 + (index*0x04),
                                    &buf[0],
                                    4);
}

/**
 * \ingroup fpgahook
 * \brief Read all hook configuration
 *
 * The buffer is required to be 32-bit aligned and a multiple of 32-bits in size.
 */

int fpga__read_hooks(uint32_t *dest)
{
    return fpga__issue_read_addr16(FPGA_SPI_CMD_READ_CTRL,
                                   0x40,
                                   dest,
                                   4*8);
}

/**
 * \ingroup fpga
 * \brief Read the current MIC idle counter
 */
int fpga__read_mic_idle()
{
    union u32 idle;

    int r = fpga__issue_read(FPGA_SPI_CMD_READ_MICIDLE, &idle.w32, 1);
    if (r<0)
        return -1;
    return idle.w8[0];
}

/**
 * \ingroup fpga
 * \brief Read the FPGA interrupt status register
 */
int fpga__readinterrupt(void)
{
    union u32 intstat;

    int r = fpga__issue_read(FPGA_SPI_CMD_READ_INTERRUPT_STATUS, &intstat.w32, 1);
    if (r<0)
        return -1;
    return intstat.w8[0];

}

/**
 * \ingroup fpga
 * \brief Acknowlede interrupt
 */
int fpga__ackinterrupt(uint8_t mask)
{
    return fpga__issue_write(FPGA_SPI_CMD_READ_INTERRUPT_ACK, &mask, 1);
}

/**
 * \ingroup fpga
 * \brief Write FPGA missctrl register
 */
int fpga__write_miscctrl(uint8_t value)
{
    return fpga__issue_write(FPGA_SPI_CMD_WRITE_MISCCTRL, &value, 1);

}
