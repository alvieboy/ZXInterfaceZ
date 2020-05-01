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
#include "esp_partition.h"
#include "esp_spi_flash.h"

static spi_device_handle_t spi0_fpga;
static fpga_flags_t latched_flags = 0;

static const uint8_t bitRevTable[256] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};


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

static int fpga__configurefromflash()
{
    const esp_partition_t *fpga_partition = NULL;
    const uint8_t *fpga_ptr = NULL;
    spi_flash_mmap_handle_t mmap_handle;



    esp_partition_iterator_t i =
        esp_partition_find(0x40, 0x00, NULL);

    if (i!=NULL) {
        fpga_partition = esp_partition_get(i);

        esp_err_t err =esp_partition_mmap(fpga_partition,
                                          0, /* Offset */
                                          fpga_partition->size,
                                          SPI_FLASH_MMAP_DATA,
                                          (const void**)&fpga_ptr,
                                          &mmap_handle);
        ESP_ERROR_CHECK(err);
        ESP_LOGI(TAG,"Mapped FPGA partition at %p", fpga_ptr);
    } else {
        ESP_LOGW(TAG,"Cannot find FPGA partition!!!");
    }
    esp_partition_iterator_release(i);

    uint32_t size = *((uint32_t*)fpga_ptr);

    if (size > fpga_partition->size) {
        ESP_LOGW(TAG, "FPGA bitfile is larger than partition size!");
        spi_flash_munmap(mmap_handle);
        return -1;
    }

    int r = fpga__passiveserialconfigure( &fpga_ptr[8], size );

    spi_flash_munmap(mmap_handle);

    return r;
}

int fpga__init()
{
    uint32_t id;
    fpga__init_spi();

    if (fpga__configurefromflash()<0)
        return -1;
    do {
        id = fpga__read_id();
    } while ((id & 0xff) == 0xff);
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

uint16_t fpga__get_spectrum_pc()
{
    uint8_t buf[3];
    buf[0] = 0x40;
    buf[1] = 0x00;
    buf[2] = 0x00;
    spi__transceive(spi0_fpga, buf, sizeof(buf));
    return getbe16(&buf[1]);
}

void fpga__set_clear_flags(fpga_flags_t enable, fpga_flags_t disable)
{
    fpga_flags_t newflags = (latched_flags & ~disable) | enable;
    uint8_t buf[4];
    buf[0] = 0xEC;
    buf[1] = newflags&0xff;
    buf[2] = 0x00;
    buf[3] = newflags>>8;
    spi__transceive(spi0_fpga, buf, sizeof(buf));
    latched_flags = newflags;
}

void fpga__set_trigger(uint8_t trig)
{
    uint8_t buf[4];
    buf[0] = 0xEC;
    buf[1] = latched_flags & 0xff;
    buf[2] = trig;
    buf[3] = latched_flags >> 8;
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
#if 0
    printf("Load res: ");
    dump__buffer(data,len);
    printf("\n");
#endif

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

        //ESP_LOGI(TAG,"Resource FIFO stat %02x avail size %d, len %d", stat, maxsize, len);

        if (maxsize > LOCAL_CHUNK_SIZE)
            maxsize = LOCAL_CHUNK_SIZE;

        if (maxsize>len)
            maxsize = len;

        //ESP_LOGI(TAG,"Resource FIFO upload size %d", maxsize);

        // Upload chunk to resource fifo.
        if (maxsize>0) {
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
        int chunk = len > sizeof(txrxbuf)?sizeof(txrxbuf):len;
        int i;
        for (i=0;i<chunk;i++) {
            txrxbuf[i] = bitRevTable[ *data++ ];
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



int fpga__read_command_fifo()
{
    uint8_t buf[3];
    buf[0] = 0xFB;
    buf[1] = 0x00;
    buf[2] = 0x00;
    spi__transceive(spi0_fpga, buf, 3);
#if 0
    printf("Cmd fifo state: ");
    dump__buffer(buf, 3);
    printf("\n");
#endif
    if (buf[1]==0xff) {
        return -1;
    }
    return buf[2];
}

uint16_t fpga__get_tap_fifo_usage()
{
    uint8_t buf[3];
    buf[0] = 0xE5;
    buf[1] = 0x00;
    buf[2] = 0x00;
    spi__transceive(spi0_fpga, buf, 3);
    return getbe16(&buf[1]);
}


int fpga__load_tap_fifo(const uint8_t *data, unsigned len, int timeout)
{
#define TAP_LOCAL_CHUNK_SIZE 256
    uint8_t txbuf[TAP_LOCAL_CHUNK_SIZE+1];

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
        txbuf[0] = 0xE4;
        memcpy(&txbuf[1], data, maxsize);
        spi__transceive(spi0_fpga, txbuf, maxsize+1);
    }

    return maxsize;
}



