#ifndef __SPI_H__
#define __SPI_H__

#include "driver/spi_master.h"
#include "driver/gpio.h"

void spi__init_bus(void);

void spi__init_device(spi_device_handle_t *dev, uint32_t speed_hz, gpio_num_t cs_pin);
int spi__transceive(spi_device_handle_t spi, uint8_t *buffer, unsigned len);

#endif
