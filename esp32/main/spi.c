#include "spi.h"
#include "defs.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t spi_sem;

void spi__init_bus()
{
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1, //PIN_NUM_QWP,
        .quadhd_io_num = -1,//PIN_NUM_QHD,
        .max_transfer_sz = 16384
    };

    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);

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

    ret=spi_bus_add_device(HSPI_HOST, &devcfg, dev);

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG,"Registered new SPI device speed=%d cs=%d", speed_hz, cs_pin);
}

int spi__transceive(spi_device_handle_t spi, uint8_t *buffer, unsigned len)
{
    spi_transaction_t t;

    if (len==0)
        return 0;             //no need to send anything

    memset(&t, 0, sizeof(t));       //Zero out the transaction

    t.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    t.rxlength = (len)*8;
    t.tx_buffer = buffer;               //Data
    t.rx_buffer = buffer;               //Data


    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &t);  //Transmit!


    xSemaphoreGive(spi_sem);
    return ret;
}

int spi__transceive_cmd8_addr24(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint32_t addr,
                                uint8_t *buffer,
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


    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!


    xSemaphoreGive(spi_sem);
    return ret;
}

int spi__transceive_cmd8_addr16(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint16_t addr,
                                uint8_t *buffer,
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


    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!


    xSemaphoreGive(spi_sem);
    return ret;
}

int spi__transceive_cmd8_addr8(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint8_t addr,
                                uint8_t *buffer,
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


    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!


    xSemaphoreGive(spi_sem);
    return ret;
}

int spi__transceive_cmd8(spi_device_handle_t spi,
                         uint8_t cmd,
                         uint8_t *buffer,
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


    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!


    xSemaphoreGive(spi_sem);
    return ret;
}

int spi__transceive_cmd8_addr32(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint32_t addr,
                                uint8_t *buffer,
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

//    ESP_LOGI(TAG, "%s: Take", __FUNCTION__);
    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!

   // ESP_LOGI(TAG, "%s: Release", __FUNCTION__);

    xSemaphoreGive(spi_sem);
    return ret;
}

int spi__transmit_cmd8_addr16(spi_device_handle_t spi,
                              uint8_t cmd,
                              uint16_t addr,
                              const uint8_t *buffer,
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


    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!


    xSemaphoreGive(spi_sem);
    return ret;
}

int spi__transmit_cmd8_addr24(spi_device_handle_t spi,
                              uint8_t cmd,
                              uint32_t addr,
                              const uint8_t *buffer,
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


    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!


    xSemaphoreGive(spi_sem);
    return ret;
}

int spi__transmit_cmd8(spi_device_handle_t spi,
                       uint8_t cmd,
                       const uint8_t *buffer,
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

    if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE(TAG, "Cannot take semaphore");
        return -1;
    }
    int ret = spi_device_polling_transmit(spi, &ext.base);  //Transmit!


    xSemaphoreGive(spi_sem);
    return ret;
}
