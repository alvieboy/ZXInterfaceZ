#ifndef __SPI_H__
#define __SPI_H__

#include "driver/spi_master.h"
#include "driver/gpio.h"

typedef uint32_t *spi_transceive_buffer_t;
typedef const uint8_t *spi_transmit_buffer_t;
typedef uint32_t *spi_receive_buffer_t;

void spi__init_bus(void);

void spi__init_device(spi_device_handle_t *dev, uint32_t speed_hz, gpio_num_t cs_pin);

int spi__transceive(spi_device_handle_t spi, spi_transceive_buffer_t buffer, unsigned len);

int spi__transceive_cmd8_addr24(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint32_t addr,
                                spi_transceive_buffer_t buffer,
                                unsigned len);

int spi__transceive_cmd8_addr8(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint8_t addr,
                                spi_transceive_buffer_t buffer,
                                unsigned len);

int spi__transceive_cmd8_addr16(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint16_t addr,
                                spi_transceive_buffer_t buffer,
                                unsigned len);

int spi__transmit_cmd8_addr16(spi_device_handle_t spi,
                              uint8_t cmd,
                              uint16_t addr,
                              spi_transmit_buffer_t buffer,
                              unsigned len);

int spi__transmit_cmd8(spi_device_handle_t spi,
                       uint8_t cmd,
                       spi_transmit_buffer_t buffer,
                       unsigned len);

int spi__transceive_cmd8(spi_device_handle_t spi,
                         uint8_t cmd,
                         spi_transceive_buffer_t buffer,
                         unsigned len);

int spi__transceive_cmd8_addr32(spi_device_handle_t spi,
                                uint8_t cmd,
                                uint32_t addr,
                                spi_transceive_buffer_t buffer,
                                unsigned len);

int spi__transmit_cmd8_addr24(spi_device_handle_t spi,
                              uint8_t cmd,
                              uint32_t addr,
                              spi_transmit_buffer_t buffer,
                              unsigned len);

#endif
