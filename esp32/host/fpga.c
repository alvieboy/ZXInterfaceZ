#include <inttypes.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "hdlc_encoder.h"
#include "hdlc_decoder.h"
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"

static xQueueHandle fpga_spi_queue = NULL;


static hdlc_encoder_t hdlc_encoder;
static hdlc_decoder_t hdlc_decoder;
static uint8_t hdlcbuf[8192];
static uint8_t writebuf[8192];
static unsigned writebufptr = 0;
static unsigned hdlcrxlen = 0;

static int emulator_socket = -1;
extern uint64_t pinstate;

void hdlc_writer(void *userdata, const uint8_t c)
{
    writebuf[writebufptr++] = c;
}

void hdlc_flusher(void *userdata)
{
    send(emulator_socket, writebuf, writebufptr, 0);
    writebufptr = 0;
}

static void hdlc_data(void *user, const uint8_t *data, unsigned datalen);

struct spi_payload
{
    uint8_t *data;
    unsigned len;
};

static void fpga_rxthread(void *arg);

int fpga_set_comms_socket(int socket)
{
    emulator_socket = socket;
}

int fpga_init()
{
    struct sockaddr_in sockaddr;
    int yes = 1;

    fpga_spi_queue = xQueueCreate(4, sizeof(struct spi_payload));

    if (emulator_socket <0) {
        emulator_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );
        if (emulator_socket<0)
            return -1;

        bzero(&sockaddr, sizeof(struct sockaddr_in));
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(8007);

        setsockopt(emulator_socket, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

        if (connect(emulator_socket, (struct sockaddr*)&sockaddr,
                    sizeof(struct sockaddr_in))<0) {
            printf("Cannot connect to ZX Spectrum emulator (QtSpecem), using "
                   "dummy FPGA emulation\n");
            close(emulator_socket);
            emulator_socket = -1;
            return 0;
        }
    }
    hdlc_decoder__init(&hdlc_decoder,
                       hdlcbuf,
                       sizeof(hdlcbuf),
                       &hdlc_data, NULL);

    hdlc_encoder__init(&hdlc_encoder, &hdlc_writer, &hdlc_flusher, NULL);

    // Start RX thread
    xTaskCreate(fpga_rxthread, "fpgathread", 4096, NULL, 6, NULL);
    return 0;
}
#if 1

void dump(const char *t, const uint8_t *buffer, size_t len)
{
    printf("%s: [",t);
    while (len--) {
        printf(" %02x", *buffer++);
    }
    printf(" ]\n");
}
#endif

extern void gpio_isr_do_trigger(gpio_num_t num);

void hdlc_data(void *user, const uint8_t *data, unsigned datalen)
{
    struct spi_payload payload;
//    printf("HDLC data %d\n", data[0]);
    switch (data[0]) {
    case 0x00:
        // Interrupt (pin) data
        printf("**** Interrupt **** source=%d\n", data[1]);
        gpio_isr_do_trigger(data[1]);

        break;
    case 0x01:
        // Spi data
        payload.data = malloc(datalen-1);
        memcpy(payload.data, &data[1], datalen-1);
        payload.len = datalen - 1;

        xQueueSendFromISR(fpga_spi_queue, &payload, NULL);

        break;
    case 0x02:
        // Raw pin data
        pinstate =
            ((uint64_t)data[1] << 56) |
            ((uint64_t)data[2] << 48) |
            ((uint64_t)data[3] << 40) |
            ((uint64_t)data[4] << 32) |
            ((uint64_t)data[5] << 24) |
            ((uint64_t)data[6] << 16) |
            ((uint64_t)data[7] << 8) |
            ((uint64_t)data[8] << 0);
        printf("**** GPIO update %016lx ****\n", pinstate);

        break;
    }
}

void fpga_rxthread(void *arg)
{
    uint8_t localbuf[512];
    hdlcrxlen = 0;
    while (1) {
        int r;
        do {
            r = recv(emulator_socket, localbuf, sizeof(localbuf), 0);
        } while ((r<0) && (errno==EINTR));
        if (r<=0) {
            printf("Error receiving: %s\n", strerror(errno));
            close(emulator_socket);
            emulator_socket = -1;
            break;
        }
        //printf("Data len %d\n", r);
        hdlc_decoder__append_buffer(&hdlc_decoder, localbuf, r);
    }
    abort();
}

void fpga_do_transaction(uint8_t *buffer, size_t len)
{
    struct spi_payload payload;

  //  dump("SPI IN: ",buffer,len);

    if (emulator_socket>=0) {
        hdlc_encoder__begin(&hdlc_encoder);
        hdlc_encoder__write(&hdlc_encoder, buffer, len);
        hdlc_encoder__end(&hdlc_encoder);
        // Wait for response
        if (!xQueueReceive(fpga_spi_queue, &payload, portMAX_DELAY)) {
            return;
        }
        // Assert payload size: TODO

        memcpy(buffer, payload.data, len);
        free(payload.data);

    } else {
        //dump("SPI IN: ",buffer,len);
        switch (buffer[0]) {
        case 0x9F:
            buffer[1] = 0xA5;
            buffer[2] = 0x00;
            buffer[3] = 0x00;
            buffer[4] = 0x00;
            break;
        }
    }
}
