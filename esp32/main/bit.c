#include "bit.h"
#include "fpga.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hdlc_decoder.h"
#include "hdlc_encoder.h"
#include "led.h"

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
        BIT_LOGI("TX %02x", c);
        fpga__write_uart_data(c);
        txlow++;
        if (txlow>=sizeof(txbuf))
            txlow = 0;
        return true;
    } else {
        return false;
    }
}

static void bit__uart_data(const uint8_t *data, int size)
{
    hdlc_decoder__append_buffer(&decoder, data,size);
}

static bool bit__process_uart_rx(uint8_t status)
{
    union {
        uint8_t w8[16];
        uint32_t w32[4];
    } rx;
    if (status & FPGA_UART_STATUS_RX_AVAIL) {
        // Extract size
        unsigned size = ((status>>2) & 0xF) +1; // 4 bits
        //BIT_LOGI("UART data size %d", size);
        int c = fpga__read_uart_data(&rx.w32[0], size);
        if (c>=0) {
            bit__uart_data(&rx.w8[0], size);
        }
        return true;
    }
    return false;
}

static void bit__uart_transmit(void *user, uint8_t c)
{
#if 0
    do {
        uint8_t status = fpga__read_uart_status();
        if (!(status & FPGA_UART_STATUS_BUSY)) {
            fpga__write_uart_data(c);
            break;
        }
    } while (1);
#endif
#if 1
    uint8_t nexttxhigh = txhigh + 1;
    if (nexttxhigh>=sizeof(txbuf))
        nexttxhigh=0;

    if (nexttxhigh!=txlow) { // Avoid overflow, discard data
        txbuf[txhigh] = c;
        txhigh = nexttxhigh;
    }
#endif
}

static void bit__uart_flush(void *user)
{
}

static void bit__transmit(const uint8_t *buffer, size_t size)
{
    hdlc_encoder__begin(&encoder);
    hdlc_encoder__write(&encoder, buffer, size);
    hdlc_encoder__end(&encoder);
}

static void bit__transmit_cmd(uint8_t data)
{
    bit__transmit(&data, 1);
}

static void bit__send_ping_reply()
{
    uint8_t txd[8];
    txd[0] = 0x01;
    uint32_t id = fpga__id();

    txd[1] = (id>>24) & 0xff;
    txd[2] = (id>>16) & 0xff;
    txd[3] = (id>>8) & 0xff;
    txd[4] = (id) & 0xff;

    bit__transmit(txd, 5);

}

static void bit__handler(void*self, const uint8_t *data, unsigned datalen)
{
    uint8_t txd[8];
    union u32 bdata;

    if (datalen<1)
        return;

    uint8_t cmd = *data++;
    datalen--;
    BIT_LOGI("Got command 0x%02x %d", cmd, datalen);
    switch (cmd) {
    case 0x01:
        // Ping
        bit__send_ping_reply();
        break;
    case 0x02:
        // Sample signals.
        txd[0] = 0x02;
        fpga__read_bit_data(&bdata.w32, 4);
        txd[1] = bdata.w8[0];
        txd[2] = bdata.w8[1];
        txd[3] = bdata.w8[2];
        txd[4] = bdata.w8[3];
        /*BIT_LOGI("BIT data: %02x %02x %02x %02x",
                 txd[1],
                 txd[2],
                 txd[3],
                 txd[4]);
          */
        bit__transmit(txd, 5);
        break;
    case 0x03:
        // Set signals.
        if (datalen==4) {
            txd[0] = 0x03;
            BIT_LOGI("BIT data: %02x %02x %02x %02x",
                     data[0],
                     data[1],
                     data[2],
                     data[3]);

            fpga__write_bit_data(data, datalen);

            bit__transmit_cmd(0x03); // Ack
        }
        break;
    case 0x04:
        // Get MAC address
        txd[0] = 0x04;
        esp_efuse_mac_get_default(&txd[1]);
        bit__transmit(txd, 7);

    }
}

/**
 * \ingroup misc
 * \brief Enter and run BIT (Built-In Test).
 *
 * This is only used for post-production checking. Do not use in normal operation.
 *
 */
void bit__run(void)
{
    txlow  = 0;
    txhigh = 0;

    BIT_LOGI("Entering BIT mode");

    hdlc_decoder__init(&decoder,
                       rxbuf,
                       sizeof(rxbuf),
                       &bit__handler,
                       NULL,
                       NULL);

    hdlc_encoder__init(&encoder,
                       &bit__uart_transmit,
                       &bit__uart_flush,
                       NULL);

    fpga__set_clear_flags(FPGA_FLAG_BITMODE,0);

    // Send out ready

    bit__send_ping_reply();

    unsigned lstatus = 0;

    while (1) {
        led__set(LED1, !!(lstatus&0x4));
        lstatus++;
        int status = fpga__read_uart_status();
        if (status>=0) {
            //if (status!=0x80)
            //    BIT_LOGI("UART Status %02x", status);
            bool processed;
            processed = bit__process_uart_rx(status);
            processed |= bit__process_uart_tx(status);
            // Each byte takes 86us
            if (!processed) {
                vTaskDelay(10 / portTICK_RATE_MS);
                //taskYIELD();
            }
        } else {
            vTaskDelay(10 / portTICK_RATE_MS);
            //taskYIELD();
        }
    }
}
