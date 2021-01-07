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
#include "freertos/semphr.h"
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "fpga.h"
#include "interface.h"
#include "usb_link.h"

static xQueueHandle fpga_spi_queue = NULL;
static SemaphoreHandle_t spi_sem;


static hdlc_encoder_t hdlc_encoder;
static hdlc_decoder_t hdlc_decoder;
static uint8_t hdlcbuf[8192];
static uint8_t writebuf[8192];
static unsigned writebufptr = 0;
static unsigned hdlcrxlen = 0;

#define FPGA_USE_SOCKET_PROTOCOL

#ifdef FPGA_USE_SOCKET_PROTOCOL
static int emulator_socket = -1;
#else
static spi_transceive_fun_t spi_transceive= NULL;
#endif

extern uint64_t pinstate;

void fpga_do_transaction(uint8_t *buffer, size_t len);

#ifndef FPGA_USE_SOCKET_PROTOCOL
int interface__set_comms_fun(spi_transceive_fun_t fun)
{
    spi_transceive = fun;
}
#endif

#ifdef FPGA_USE_SOCKET_PROTOCOL
static void hdlc_writer(void *userdata, const uint8_t c)
{
    writebuf[writebufptr++] = c;
}

static void hdlc_flusher(void *userdata)
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
#endif


int fpga_init(void);

int fpga_init(void)
{
#ifdef FPGA_USE_SOCKET_PROTOCOL
    struct sockaddr_in sockaddr;
    int yes = 1;

    fpga_spi_queue = xQueueCreate(4, sizeof(struct spi_payload));
    spi_sem = xSemaphoreCreateMutex();

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
                       &hdlc_data,
                       NULL,
                       NULL);

    hdlc_encoder__init(&hdlc_encoder, &hdlc_writer, &hdlc_flusher, NULL);
    printf("Starting FPGA thread\n");
    // Start RX thread
    xTaskCreate(fpga_rxthread, "fpgathread", 4096, NULL, 6, NULL);
#else
    // Ignore
#endif
    printf("FPGA ready\n");
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

void interface__gpio_trigger(int num)
{
    gpio_isr_do_trigger(num);
}

void interface__rawpindata(uint64_t p)
{
    pinstate = p;
}

void interface__connectusb(const char *id)
{
    usb_attach("0e8f:0003");
}

#ifdef FPGA_USE_SOCKET_PROTOCOL
void hdlc_data(void *user, const uint8_t *data, unsigned datalen)
{
    struct spi_payload payload;
#if 0
    dump("SPI IN (via hdlc): ",data, datalen);
#endif
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
    case 0x03:
        printf("USB connection\n");
        uint8_t len = data[1];
        char *newid = malloc(len);
        strncpy(newid, &data[2], len);
        interface__connectusb(newid);
        break;
    }
}

void fpga_rxthread(void *arg)
{
    uint8_t localbuf[512];
    hdlcrxlen = 0;

    fcntl(emulator_socket, F_SETFL, fcntl(emulator_socket, F_GETFL) | O_NONBLOCK);

    while (1) {
        int r;
        do {
            r = recv(emulator_socket, localbuf, sizeof(localbuf), 0);
            if (r<0 && errno==EAGAIN) {
                vTaskDelay(10 / portTICK_RATE_MS);
            }
        } while ((r<0) && (errno==EINTR || errno==EAGAIN));
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

#endif

void fpga_do_transaction(uint8_t *buffer, size_t len)
{

#ifdef FPGA_USE_SOCKET_PROTOCOL

    struct spi_payload payload;

    if (emulator_socket>=0) {

        // Mutex.
        if (xSemaphoreTake( spi_sem,  portMAX_DELAY )!= pdTRUE) {
            printf("Cannot take semaphore!!!\n");
            return -1;
        }

        hdlc_encoder__begin(&hdlc_encoder);
        hdlc_encoder__write(&hdlc_encoder, buffer, len);
        hdlc_encoder__end(&hdlc_encoder);
        // Wait for response
        if (!xQueueReceive(fpga_spi_queue, &payload, portMAX_DELAY)) {
            xSemaphoreGive(spi_sem);
            return;
        }
        xSemaphoreGive(spi_sem);
        // Assert payload size: TODO
#if 0
        printf("SPI req %02x reply %02x\n", buffer[0], payload.data[0]);
#endif
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
#else
    if (spi_transceive)
        spi_transceive(buffer, &buffer[1], len);
#endif
}
