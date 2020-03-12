#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "defs.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "spi.h"
#include "dump.h"
#include "fpga.h"
#include "hdlc_decoder.h"
#include "hdlc_encoder.h"
#include <string.h>

static spi_device_handle_t spi0_as_flash;

static int flash_pgm__as_flash_init()
{
    uint8_t idbuf[5];
    idbuf[0] = 0x9F;
    idbuf[1] = 0xAA;
    idbuf[2] = 0x55;
    idbuf[3] = 0xAA;
    idbuf[4] = 0x55;

    int r = spi__transceive(spi0_as_flash,idbuf,5);
    if (r<0) {
        printf("SPI transceive: error %d\r\n", r);
        return r;
    } else {
        printf("SPI Flash id: ");
        dump__buffer(&idbuf[1], 4);
        printf("\r\n");
    }
    return 0;
}

static uint8_t hdlc_rxbuf[1024];
static uint8_t hdlc_txbuf[1024];

#define VERSION_HIGH 0x01
#define VERSION_LOW  0x0A

/* Commands for programmer */

#define BOOTLOADER_CMD_VERSION 0x01
#define BOOTLOADER_CMD_IDENTIFY 0x02
#define BOOTLOADER_CMD_WAITREADY 0x03
#define BOOTLOADER_CMD_RAWREADWRITE 0x04
#define BOOTLOADER_CMD_ENTERPGM 0x05
#define BOOTLOADER_CMD_LEAVEPGM 0x06
#define BOOTLOADER_CMD_SSTAAIPROGRAM 0x07
#define BOOTLOADER_CMD_SETBAUDRATE 0x08
#define BOOTLOADER_CMD_PROGMEM 0x09
#define BOOTLOADER_CMD_START 0x0A
#define BOOTLOADER_MAX_CMD 0x0A

#ifdef SIMULATION
# define BOOTLOADER_WAIT_MILLIS 10
#else
# define BOOTLOADER_WAIT_MILLIS 1000
#endif

#define REPLY(X) (X|0x80)

static uint8_t *simpleReply(hdlc_encoder_t *enc, uint8_t *buf, unsigned int r)
{
    uint8_t *ret = hdlc_encoder__begin_mem(enc, buf);
    uint8_t v = REPLY(r);
    ret = hdlc_encoder__write_mem_byte(enc, v, ret);

    ret = hdlc_encoder__end_mem(enc, ret);
    return ret;
}

static int spi_read_status()
{
    unsigned int status;
    uint8_t buf[2];
    buf[0] = 0x05;
    buf[1] = 0x00;

    spi__transceive(spi0_as_flash, buf, sizeof(buf));


    status =  buf[1];

    return status;
}

unsigned int fpga_pgm__read_id()
{
    unsigned int ret;
    uint8_t buf[4];
    buf[0] = 0x9f;
    spi__transceive(spi0_as_flash, buf, sizeof(buf));
    ret = (((unsigned)buf[1])<<16)
        | (((unsigned)buf[2])<<8)
        | (((unsigned)buf[3]));

    return ret;
}

static uint8_t *cmd_progmem(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    return buf;
}

static uint8_t spibuf[1024];

static uint8_t *cmd_raw_send_receive(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    unsigned int count;
    unsigned int rxcount;
    unsigned int txcount;

    // buffer[1-2] is number of TX bytes
    // buffer[3-4] is number of RX bytes
    // buffer[5..] is data to transmit.

    // NOTE - buffer will be overwritten in read.

    //spi_enable();
    txcount = data[1];
    txcount<<=8;
    txcount += data[2];
    rxcount = data[3];
    rxcount<<=8;
    rxcount += data[4];

    if (txcount+rxcount > sizeof(spibuf)) {
        return buf;
    }

    for (count=0; count<txcount; count++) {
        //spiwrite(spidata,buffer[5+count]);
        spibuf[count] = data[5+count];
    }
    uint8_t *startrx = &spibuf[count];

    count += rxcount;


    // Now, receive and write buffer
    /*for(count=0;count <rxcount;count++) {
        spiwrite(spidata,0x00);
        buffer[count] = spiread(spidata);
    }
    spi_disable(spidata);
    */
    spi__transceive(spi0_as_flash, &spibuf[0], count);

    // Send back
    uint8_t *ret = hdlc_encoder__begin_mem(enc, buf);

    ret = hdlc_encoder__write_mem_byte(enc, REPLY(BOOTLOADER_CMD_RAWREADWRITE),  ret);
    ret = hdlc_encoder__write_mem_byte(enc, rxcount>>8,  ret);
    ret = hdlc_encoder__write_mem_byte(enc, rxcount,  ret);

    ret = hdlc_encoder__write_mem(enc, startrx, rxcount,  ret);

    ret = hdlc_encoder__end_mem(enc, ret);
    return ret;
}


static uint8_t *cmd_sst_aai_program(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    return buf;
}

static uint8_t *cmd_set_baudrate(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    return simpleReply(enc, buf, BOOTLOADER_CMD_SETBAUDRATE);
}

static int spi_flash_wait_ready()
{
    int timeout = 100000;
    uint8_t status;
    do {
        status = spi_read_status();
        timeout--;
    } while ((timeout>0) && (status & 1));

    if (timeout==0)
        return -1;
    return 0;
}

static uint8_t *cmd_waitready(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    int status;
    int timeout = 100000;
    do {
        status = spi_read_status();
        timeout--;
    } while ((timeout>0) && (status & 1));

    if (timeout==0) {
        ESP_LOGE(TAG, "Flash ready timeout, status %02x\r\n", status);
        return buf;
    } else {
        uint8_t *ret = hdlc_encoder__begin_mem(enc, buf);
        uint8_t v[2];
        v[0] = REPLY(BOOTLOADER_CMD_WAITREADY);
        v[1] = status;
        ret = hdlc_encoder__write_mem(enc, &v[0], sizeof(v), ret);

        ret = hdlc_encoder__end_mem(enc, ret);
        return ret;
    }
}

#define BOARD_ID 0xBD010100UL
#define CLK_FREQ 80000000


static unsigned char vstring[] = {
	VERSION_HIGH,
	VERSION_LOW,
	0,//SPIOFFSET>>16,
	0,//SPIOFFSET>>8,
	0,//SPIOFFSET&0xff,
	0,
	0,
	0,
	(CLK_FREQ >> 24) & 0xff,
	(CLK_FREQ >> 16) & 0xff,
	(CLK_FREQ >> 8) & 0xff,
	(CLK_FREQ) & 0xff,
	(BOARD_ID >> 24) & 0xff,
	(BOARD_ID >> 16) & 0xff,
	(BOARD_ID >> 8) & 0xff,
        (BOARD_ID) & 0xff,
        0,
        0,
        0,
        0  /* Memory top, to pass on to application */
};

static uint8_t *cmd_version(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    uint8_t *ret = hdlc_encoder__begin_mem(enc, buf);
    uint8_t v = REPLY(BOOTLOADER_CMD_VERSION);

    ret = hdlc_encoder__write_mem_byte(enc, v, ret);
    ret = hdlc_encoder__write_mem(enc, vstring, sizeof(vstring), ret);
    ret = hdlc_encoder__end_mem(enc, ret);

    return ret;
}

static uint8_t *cmd_identify(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    // Reset boot counter
    int id;
    uint8_t *ret = hdlc_encoder__begin_mem(enc, buf);

    ret = hdlc_encoder__write_mem_byte(enc, REPLY(BOOTLOADER_CMD_IDENTIFY), ret);
    unsigned flash_id = fpga_pgm__read_id();
    ret = hdlc_encoder__write_mem_byte(enc, flash_id>>16, ret);
    ret = hdlc_encoder__write_mem_byte(enc, flash_id>>8, ret);
    ret = hdlc_encoder__write_mem_byte(enc, flash_id, ret);

    id = spi_read_status();
    ret = hdlc_encoder__write_mem_byte(enc, id, ret);

    ret = hdlc_encoder__end_mem(enc, ret);

    return ret;
}


static uint8_t *cmd_enterpgm(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    return simpleReply(enc, buf, BOOTLOADER_CMD_ENTERPGM);
}

static uint8_t *cmd_leavepgm(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    // Trigger reconfiguration
    ESP_LOGI(TAG, "Triggering FPGA reconfiguration");
    fpga__trigger_reconfiguration();
    return simpleReply(enc, buf, BOOTLOADER_CMD_LEAVEPGM);
}
 

static uint8_t *cmd_start(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data)
{
    return buf;
}


typedef uint8_t*(*cmdhandler_t)(hdlc_encoder_t *enc, uint8_t *buf, const unsigned char *data);

static const cmdhandler_t handlers[] = {
    &cmd_version,         /* CMD1 */
    &cmd_identify,        /* CMD2 */
    &cmd_waitready,       /* CMD3 */
    &cmd_raw_send_receive,/* CMD4 */
    &cmd_enterpgm,        /* CMD5 */
    &cmd_leavepgm,        /* CMD6 */
    &cmd_sst_aai_program, /* CMD7 */
    &cmd_set_baudrate,    /* CMD8 */
    &cmd_progmem,         /* CMD9 */
    &cmd_start            /* CMD10 */
};

static void hdlc_handler(void *user, const uint8_t *data, unsigned datalen)
{
    hdlc_encoder_t enc;

    int sock = *(int*)user;

    if (datalen<1)
        return;

    uint8_t pos = data[0];
    if (pos==0)
        return;

    //printf("HDLC: command 0x%02x\r\n", pos);

    if (pos>BOOTLOADER_MAX_CMD)
        return;
    pos--;

    uint8_t *end = handlers[pos](&enc, hdlc_txbuf, data);
    if (end!=NULL) {
        if (end!=hdlc_txbuf) {
            //printf("HDLC: Sending reply size %d\n", end - hdlc_txbuf);
            send(sock, hdlc_txbuf, end - hdlc_txbuf, 0);
        }
    }
}

static void flash_pgm__task(void *pvParameters)
{
    uint8_t rx_buffer[512];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(CMD_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_TCP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);

        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket bound, port %d", CMD_PORT);

        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket listening");


        while (1) {
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            uint addr_len = sizeof(source_addr);
            int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);


            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                break;
            }
            ESP_LOGI(TAG, "Socket accepted");

            while (1) {
                hdlc_decoder_t dec;
                hdlc_decoder__init( &dec, hdlc_rxbuf, sizeof(hdlc_rxbuf),
                                   &hdlc_handler, &sock);

                int len = recv(sock, rx_buffer, sizeof(rx_buffer), 0);
                // Error occurred during receiving
                if (len < 0) {
                    ESP_LOGE(TAG, "recv failed: errno %d", errno);
                    break;
                }
                // Connection closed
                else if (len == 0) {
                    ESP_LOGI(TAG, "Connection closed");
                    break;
                }
                // Data received
                else {
                    hdlc_decoder__append_buf(&dec, rx_buffer, len);
                    err = 0;
                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
                }
            }
            shutdown(sock,0);
            close(sock);
            sock = -1;
        }
    }
    vTaskDelete(NULL);
}


void flash_pgm__init()
{
    spi__init_device(&spi0_as_flash, 10000000, PIN_NUM_AS_CSN);

    flash_pgm__as_flash_init();

    xTaskCreate(flash_pgm__task, "cmd_server", 4096, NULL, 5, NULL);
}


static int flash_pgm__enable_writes()
{
    unsigned char buf[1];

    buf[0] = 0x06; // Enable Write

    int r = spi__transceive(spi0_as_flash, buf, sizeof(buf));

    return r;
}


int flash_pgm__erase_sector_address(int sector_address)
{
    uint8_t buf[4];
    int r;

    buf[0] = 0xD8; // Sector erase
    buf[1] = (sector_address>>16) & 0xff;
    buf[2] = (sector_address>>8) & 0xff;
    buf[3] = (sector_address) & 0xff;

    ESP_LOGI(TAG, "Erasing sector at 0x%08x", sector_address);

    if (spi_flash_wait_ready()<0)
        return -1;

    if (flash_pgm__enable_writes()<0)
        return -1;

    r = spi__transceive(spi0_as_flash,buf, 4);

    if (r<0)
        return -1;

    if (spi_flash_wait_ready()<0)
        return -1;

    return 0;
}

int flash_pgm__program_page( int address, const uint8_t *data, int size)
{
    uint8_t buf[256 + 4];
    int r;

    if (size>256)
        return -1;

    buf[0] = 0x02; // Page program
    buf[1] = (address >> 16) & 0xff;
    buf[2] = (address >> 8) & 0xff;
    buf[3] = (address) & 0xff;

    memcpy(&buf[4], data, size);

    ESP_LOGI(TAG, "Programming page at 0x%08x len %d", address, size);

    if (flash_pgm__enable_writes()<0)
        return -1;

    r = spi__transceive(spi0_as_flash, buf, size+4);

    if (r<0)
        return -1;

    if (spi_flash_wait_ready()<0)
        return -1;

    return 0;
}

