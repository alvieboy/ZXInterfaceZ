#include "fpga.h"
#include "spi.h"
#include "defs.h"
#include "dump.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static spi_device_handle_t spi0_fpga;
static uint8_t latched_flags = 0;

static void fpga__init_spi()
{
    spi__init_device(&spi0_fpga, 20000000, PIN_NUM_CS);
}

int fpga__init()
{
    uint8_t idbuf[5];
    int r;
    fpga__init_spi();

    do {
        idbuf[0] = 0x9F;
        idbuf[1] = 0xAA;
        idbuf[2] = 0x55;
        idbuf[3] = 0xAA;
        idbuf[4] = 0x55;


        r = spi__transceive(spi0_fpga, idbuf, 5);

        if (r<0) {
            printf("SPI transceive: error %d\r\n", r);
        } else {
            printf("FPGA id: ");
            dump__buffer(&idbuf[1], 4);
            printf("\r\n");
            if (idbuf[1] == 0xA5) {
                break;
            }
        }
    } while (0);
    return r;
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
#define LOCAL_CHUNK_SIZE 256
    uint8_t txbuf[LOCAL_CHUNK_SIZE+1];

    do {
        uint8_t stat = fpga__get_status();
        unsigned maxsize;
        if (stat & FPGA_STATUS_RESFIFO_FULL) {
            maxsize=0;
        } else if (stat & FPGA_STATUS_RESFIFO_QQQFULL) {
            maxsize = 0;//(FPGA_RESOURCE_FIFO_SIZE/4) -1;
        } else if (stat & FPGA_STATUS_RESFIFO_HFULL) {
            maxsize = (FPGA_RESOURCE_FIFO_SIZE/2) -1;
        } else if (stat & FPGA_STATUS_RESFIFO_QFULL) {
            maxsize = (FPGA_RESOURCE_FIFO_SIZE*3/4) -1;
        } else {
            maxsize = FPGA_RESOURCE_FIFO_SIZE-1;
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

