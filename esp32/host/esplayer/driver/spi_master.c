#include "driver/spi_master.h"
#include <malloc.h>
#include <unistd.h>
#include <string.h>

extern void fpga_do_transaction(uint8_t *buffer, size_t len);

esp_err_t spi_bus_initialize(spi_host_device_t host_id, const spi_bus_config_t *bus_config, int dma_chan)
{
    return ESP_OK;
}


esp_err_t spi_bus_add_device(spi_host_device_t host_id, const spi_device_interface_config_t *dev_config, spi_device_handle_t *handle)
{
    return ESP_OK;
}

esp_err_t spi_device_polling_transmit(spi_device_handle_t handle, spi_transaction_t *trans_desc)
{
    spi_transaction_ext_t *ext = (spi_transaction_ext_t*)trans_desc;

    //printf("SPI len %d flags %d\n", ext->base.length, ext->base.flags);

    unsigned txlen = (ext->base.length/8);
    // Re-allocate so we can merge everyting
    uint8_t *transaction = malloc( txlen + 16);
    uint8_t *tptr = transaction;

    if (ext->base.flags & SPI_TRANS_VARIABLE_CMD) {
        int i;
        uint32_t command = ext->base.cmd;

        for (i = ext->command_bits/8; i>0; i--) {
            *tptr++ = (command>>(8*(i-1))) & 0xff;
            txlen++;
        }
    }

    if (ext->base.flags & SPI_TRANS_VARIABLE_ADDR) {
        int i;
        uint32_t addr = ext->base.addr;

        for (i = ext->address_bits/8; i>0; i--) {
            *tptr++ = (addr>>(8*(i-1))) & 0xff;
            txlen++;
        }
    }
    memcpy(tptr, ext->base.tx_buffer, ext->base.length/8);

    do {
        printf("SPI TX: [ ");
        for (int z=0;z<txlen;z++) {
            if (tptr == &transaction[z]) {
                printf("$ ");
            }
            printf("%02x ", transaction[z]);
        }
        printf("]\n");
    } while (0);


    fpga_do_transaction( transaction, txlen );



    if (ext->base.rx_buffer) {
        memcpy(ext->base.rx_buffer, tptr, ext->base.rxlength/8);
    }

    free(transaction);
    return 0;
    /*ext.base.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    ext.base.rxlength = (len)*8;
    ext.base.tx_buffer = buffer;               //Data
    ext.base.rx_buffer = buffer;               //Data
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;

    ext.command_bits = 8;
    ext.address_bits = 24;
    ext.base.cmd = (uint16_t)cmd;
    ext.base.addr = addr;
     */

    return -1;
}
