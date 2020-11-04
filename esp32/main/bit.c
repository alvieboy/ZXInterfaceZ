#include "bit.h"
#include "fpga.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hdlc_decoder.h"
#include "hdlc_encoder.h"

#define BIT_LOGI(x...) ESP_LOGI("BIT", x)

static uint8_t txbuf[16];
static uint8_t rxbuf[16];
static uint8_t txlow, txhigh;
static hdlc_decoder_t decoder;
static hdlc_encoder_t encoder;


static bool bit__process_uart_tx(uint8_t status)
{
    if (txlow!=txhigh) {
        if (status & FPGA_UART_STATUS_BUSY)
            return false;
        uint8_t c = txbuf[txlow];
        fpga__write_uart_data(c);
        txlow++;
        if (txlow>=sizeof(txbuf))
            txlow = 0;
        return true;
    } else {
        return false;
    }
}

static void bit__uart_data(uint8_t value)
{
    hdlc_decoder__append(&decoder, value);
}

static bool bit__process_uart_rx(uint8_t status)
{
    if (status & FPGA_UART_STATUS_RX_AVAIL) {
        int c = fpga__read_uart_data();
        if (c>=0) {
            bit__uart_data(c&0xff);
        }
        return true;
    }
    return false;
}

static void bit__uart_transmit(void *user, uint8_t c)
{
    uint8_t nexttxhigh = txhigh + 1;
    if (nexttxhigh>sizeof(txbuf))
        nexttxhigh=0;

    if (nexttxhigh!=txlow) { // Avoid overflow, discard data
        txbuf[txhigh] = c;
        txhigh = nexttxhigh;
    }
}

static void bit__uart_flush(void *user)
{
}

static void bit__transmit(const uint8_t *buffer, size_t size)
{
    hdlc_encoder__write(&encoder, buffer, size);
}

static void bit__transmit_cmd(uint8_t data)
{
    bit__transmit(&data, 1);
}

static void bit__handler(void*self, const uint8_t *data, unsigned datalen)
{
    uint8_t txd[8];

    if (datalen<1)
        return;

    uint8_t cmd = *data++;
    datalen--;

    switch (cmd) {
    case 0x01:
        // Ping
        bit__transmit_cmd(0x01);
        break;
    case 0x02:
        // Sample signals.
        txd[0] = 0x02;
        fpga__read_bit_data(&txd[1], 4);
        bit__transmit(txd, 5);
        break;
    case 0x03:
        // Set signals.
        if (datalen==2) {
            txd[0] = 0x03;
            fpga__write_bit_data(data, datalen);
            bit__transmit_cmd(0x03); // Ack
        }
        break;

    }
}


void bit__run(void)
{
    txlow  = 0;
    txhigh = 0;

    BIT_LOGI("Entering BIT mode");

    hdlc_decoder__init(&decoder,
                       rxbuf,
                       sizeof(rxbuf),
                       &bit__handler,
                       NULL);

    hdlc_encoder__init(&encoder,
                       &bit__uart_transmit,
                       &bit__uart_flush,
                       NULL);

    fpga__set_clear_flags(FPGA_FLAG_BITMODE,0);

    // Send out ready

    bit__transmit_cmd(0x01);

    while (1) {
        int status = fpga__read_uart_status();
        if (status>=0) {
            bool processed;
            processed = bit__process_uart_rx(status);
            processed |= bit__process_uart_tx(status);
            if (!processed) {
                vTaskDelay(10 / portTICK_RATE_MS);
            }
        } else {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
    }
}
