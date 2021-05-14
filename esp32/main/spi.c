#include "spi.h"
#include "defs.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define TAG "SPI"

static SemaphoreHandle_t spi_sem;

#define xSPI_HOST VSPI_HOST

void spi__init_bus()
{
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1, //PIN_NUM_QWP,
        .quadhd_io_num = -1,//PIN_NUM_QHD,
        .flags = SPICOMMON_BUSFLAG_MASTER
        | SPICOMMON_BUSFLAG_IOMUX_PINS
        | SPICOMMON_BUSFLAG_SCLK
        | SPICOMMON_BUSFLAG_MISO
        | SPICOMMON_BUSFLAG_MOSI
        ,
        .max_transfer_sz = 16384
    };

    ret=spi_bus_initialize(xSPI_HOST, &buscfg, 1);

    ESP_ERROR_CHECK(ret);

    spi_sem = xSemaphoreCreateMutex();
    if (spi_sem==NULL) {
        ESP_ERROR_CHECK(-1);
    }
}

void spi__init_device(spi_device_handle_t *dev, uint32_t speed_hz, gpio_num_t cs_pin)
{
    esp_err_t ret;

    spi_device_interface_config_t devcfg = {0};

    devcfg.clock_speed_hz=speed_hz;
    devcfg.mode = 3;
    devcfg.spics_io_num = cs_pin;
    devcfg.queue_size = 1;

    ret=spi_bus_add_device(xSPI_HOST, &devcfg, dev);

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG,"Registered new SPI device speed=%d cs=%d", speed_hz, cs_pin);
}

static int spi__lock()
{
    int r = xSemaphoreTake( spi_sem,  portMAX_DELAY );
    if (r!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take SPI semaphore: %d %d", r, pdTRUE);
        return -1;
    }
    return 0;
}
static void spi__unlock()
{
    xSemaphoreGive(spi_sem);
}

#define SPI_LOCK(x) do { if (spi__lock()<0) return -1; } while (0)
#define SPI_UNLOCK spi__unlock

int spi__transceive(spi_device_handle_t spi, spi_transceive_buffer_t buffer, unsigned len)
{
    spi_transaction_t t;

    if (len==0)
        return 0;             //no need to send anything

    memset(&t, 0, sizeof(t));       //Zero out the transaction

    t.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    t.rxlength = (len)*8;
    t.tx_buffer = buffer;               //Data
    t.rx_buffer = buffer;               //Data

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &t);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }
    return ret;
}

int spi__transceive_cmd8_addr24(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint32_t addr,
                                spi_transceive_buffer_t buffer,
                                unsigned len)
{
    spi_transaction_ext_t ext;

    if (len==0)
        return 0;             //no need to send anything

    memset(&ext, 0, sizeof(ext));       //Zero out the transaction

    ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = (len)*8;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = buffer;               //Data
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    ext.command_bits = 8;
    ext.address_bits = 24;
    ext.base.cmd = (uint16_t)cmd;
    ext.base.addr = addr;

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }

    return ret;
}

int spi__transceive_cmd8_addr16(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint16_t addr,
                                spi_transceive_buffer_t buffer,
                                unsigned len)
{
    spi_transaction_ext_t ext;

    if (len==0)
        return 0;             //no need to send anything

    memset(&ext, 0, sizeof(ext));       //Zero out the transaction

    ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = (len)*8;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = buffer;               //Data
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    ext.command_bits = 8;
    ext.address_bits = 16;
    ext.base.cmd = (uint16_t)cmd;
    ext.base.addr = addr;

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }

    return ret;
}

int spi__transceive_cmd8_addr8(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint8_t addr,
                               spi_transceive_buffer_t buffer,
                                unsigned len)
{
    spi_transaction_ext_t ext;

    if (len==0)
        return 0;             //no need to send anything

    memset(&ext, 0, sizeof(ext));       //Zero out the transaction

    ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = (len)*8;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = buffer;               //Data
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    ext.command_bits = 8;
    ext.address_bits = 8;
    ext.base.cmd = (uint16_t)cmd;
    ext.base.addr = addr;

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }

    return ret;
}

int spi__transceive_cmd8(spi_device_handle_t spi,
                         uint8_t cmd,
                         spi_transceive_buffer_t buffer,
                         unsigned len)
{
    spi_transaction_ext_t ext;

    if (len==0)
        return 0;             //no need to send anything

    memset(&ext, 0, sizeof(ext));       //Zero out the transaction

    ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = (len)*8;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = buffer;               //Data
    ext.base.flags = SPI_TRANS_VARIABLE_CMD;
    ext.command_bits = 8;
    ext.base.cmd = (uint16_t)cmd;

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }

    return ret;
}

int spi__transceive_cmd8_addr32(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint32_t addr,
                                spi_transceive_buffer_t buffer,
                                unsigned len)
{
    spi_transaction_ext_t ext;

    if (len==0)
        return 0;             //no need to send anything

    memset(&ext, 0, sizeof(ext));       //Zero out the transaction

    ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = (len)*8;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = buffer;               //Data
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    ext.command_bits = 8;
    ext.address_bits = 32;
    ext.base.cmd = (uint16_t)cmd;
    ext.base.addr = addr;

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }

    return ret;
}

int spi__transmit_cmd8_addr16(spi_device_handle_t spi,
                              uint8_t cmd,
                              uint16_t addr,
                              spi_transmit_buffer_t buffer,
                              unsigned len)
{
    spi_transaction_ext_t ext;

    if (len==0)
        return 0;             //no need to send anything

    memset(&ext, 0, sizeof(ext));       //Zero out the transaction

    ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = 0;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = NULL;
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    ext.command_bits = 8;
    ext.address_bits = 16;
    ext.base.cmd = (uint16_t)cmd;
    ext.base.addr = addr;

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }

    return ret;
}

int spi__transmit_cmd8_addr24(spi_device_handle_t spi,
                              uint8_t cmd,
                              uint32_t addr,
                              spi_transmit_buffer_t buffer,
                              unsigned len)
{
    spi_transaction_ext_t ext;

    if (len==0)
        return 0;             //no need to send anything

    memset(&ext, 0, sizeof(ext));       //Zero out the transaction

    ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = 0;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = NULL;
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    ext.command_bits = 8;
    ext.address_bits = 24;
    ext.base.cmd = (uint16_t)cmd;
    ext.base.addr = addr;

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }

    return ret;
}

int spi__transmit_cmd8(spi_device_handle_t spi,
                       uint8_t cmd,
                       spi_transmit_buffer_t buffer,
                       unsigned len)
{
    spi_transaction_ext_t ext;

    if (len==0)
        return 0;             //no need to send anything

    memset(&ext, 0, sizeof(ext));       //Zero out the transaction

    ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = 0;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = NULL;
    ext.base.flags = SPI_TRANS_VARIABLE_CMD;
    ext.command_bits = 8;
    ext.address_bits = 0;
    ext.base.cmd = (uint16_t)cmd;

    SPI_LOCK();
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!
    SPI_UNLOCK();

    if (ret<0) {
        ESP_LOGE(TAG, "Cannot SPI transmit!!");
    }

    return ret;
}
