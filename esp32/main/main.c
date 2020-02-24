/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/spi_master.h"
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
#include "hdlc_decoder.h"
#include "hdlc_encoder.h"

#define CMD_PORT 8002

static spi_device_handle_t spi0_fpga;
static spi_device_handle_t spi0_as_flash;


static void spi__init(spi_device_handle_t *fpga_spi,
                      spi_device_handle_t *as_flash_spi)
{
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = PIN_NUM_QWP,
        .quadhd_io_num = PIN_NUM_QHD,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t devcfg={
        .clock_speed_hz=10*1000*1000,
        .mode = 3,                                //SPI mode 0
        .spics_io_num = PIN_NUM_CS,               //CS pin
        .queue_size = 2,                          //We want to be able to queue 7 transactions at a time
        .pre_cb = NULL,
    };

    spi_device_interface_config_t devcfg_as={
        .clock_speed_hz=10*1000*1000,
        .mode = 3,                                //SPI mode 3
        .spics_io_num = PIN_NUM_AS_CSN,               //CS pin
        .queue_size = 1,                          //We want to be able to queue 7 transactions at a time
        .pre_cb = NULL,
    };

    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);

    ret=spi_bus_add_device(HSPI_HOST, &devcfg, fpga_spi);
    ESP_ERROR_CHECK(ret);

    ret=spi_bus_add_device(HSPI_HOST, &devcfg_as, as_flash_spi);
    ESP_ERROR_CHECK(ret);
}


#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "wifi softAP";

static void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
    IP4_ADDR(&info.ip, 192, 168, 120, 1);
    IP4_ADDR(&info.gw, 192, 168, 120, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

static void wifi__init()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
}

void sdcard__init()
{
    ESP_LOGI(TAG, "Using SDMMC peripheral");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.gpio_cd = SDMMC_PIN_DET;

    // To use 1-line SD mode, uncomment the following line:
    // slot_config.width = 1;

    gpio_set_pull_mode(SDMMC_PIN_CMD, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(SDMMC_PIN_D0, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(SDMMC_PIN_D1, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(SDMMC_PIN_D2, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(SDMMC_PIN_D3, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes


    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
}

static void gpio__init()
{
    gpio_config_t io_conf;

    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT_OD;
    io_conf.pin_bit_mask = (1ULL<<PIN_NUM_LED1) | (1ULL<<PIN_NUM_LED2);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en   = 0;

    gpio_config(&io_conf);

    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        (1ULL<<PIN_NUM_AS_CSN);
    gpio_config(&io_conf);

    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
        (1ULL<<PIN_NUM_SWITCH) |
        (1ULL<<PIN_NUM_IO25) |
        (1ULL<<PIN_NUM_IO26) |
        (1ULL<<PIN_NUM_IO14);

    io_conf.pull_up_en   = 1;

    gpio_config(&io_conf);


}

typedef enum {
    LED1,
    LED2
} led_t;

static void led__set(led_t led, uint8_t on)
{
    gpio_num_t gpio;
    if (led==LED1) {
        gpio = PIN_NUM_LED1;
    } else {
        gpio = PIN_NUM_LED2;
    }
    gpio_set_level(gpio, !on);

}

static int spi__transceive(spi_device_handle_t spi, uint8_t *buffer, unsigned len)
{
    spi_transaction_t t;

#if 0
#define SPI_TRANS_MODE_DIO            (1<<0)  ///< Transmit/receive data in 2-bit mode
#define SPI_TRANS_MODE_QIO            (1<<1)  ///< Transmit/receive data in 4-bit mode
#define SPI_TRANS_USE_RXDATA          (1<<2)  ///< Receive into rx_data member of spi_transaction_t instead into memory at rx_buffer.
#define SPI_TRANS_USE_TXDATA          (1<<3)  ///< Transmit tx_data member of spi_transaction_t instead of data at tx_buffer. Do not set tx_buffer when using this.
#define SPI_TRANS_MODE_DIOQIO_ADDR    (1<<4)  ///< Also transmit address in mode selected by SPI_MODE_DIO/SPI_MODE_QIO
#define SPI_TRANS_VARIABLE_CMD        (1<<5)  ///< Use the ``command_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.
#define SPI_TRANS_VARIABLE_ADDR       (1<<6)  ///< Use the ``address_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.
#define SPI_TRANS_VARIABLE_DUMMY      (1<<7)  ///< Use the ``dummy_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.

    /**
     * This structure describes one SPI transaction. The descriptor should not be modified until the transaction finishes.
     */
    struct spi_transaction_t {
        uint32_t flags;                 ///< Bitwise OR of SPI_TRANS_* flags
        uint16_t cmd;                   /**< Command data, of which the length is set in the ``command_bits`` of spi_device_interface_config_t.
        *
        *  <b>NOTE: this field, used to be "command" in ESP-IDF 2.1 and before, is re-written to be used in a new way in ESP-IDF 3.0.</b>
        *
        *  Example: write 0x0123 and command_bits=12 to send command 0x12, 0x3_ (in previous version, you may have to write 0x3_12).
        */
        uint64_t addr;                  /**< Address data, of which the length is set in the ``address_bits`` of spi_device_interface_config_t.
        *
        *  <b>NOTE: this field, used to be "address" in ESP-IDF 2.1 and before, is re-written to be used in a new way in ESP-IDF3.0.</b>
        *
        *  Example: write 0x123400 and address_bits=24 to send address of 0x12, 0x34, 0x00 (in previous version, you may have to write 0x12340000).
        */
        size_t length;                  ///< Total data length, in bits
        size_t rxlength;                ///< Total data length received, should be not greater than ``length`` in full-duplex mode (0 defaults this to the value of ``length``).
        void *user;                     ///< User-defined variable. Can be used to store eg transaction ID.
        union {
            const void *tx_buffer;      ///< Pointer to transmit buffer, or NULL for no MOSI phase
            uint8_t tx_data[4];         ///< If SPI_USE_TXDATA is set, data set here is sent directly from this variable.
        };
        union {
            void *rx_buffer;            ///< Pointer to receive buffer, or NULL for no MISO phase. Written by 4 bytes-unit if DMA is used.
            uint8_t rx_data[4];         ///< If SPI_USE_RXDATA is set, data is received directly to this variable
        };
    } ;        //the rx data should start from a 32-bit aligned address to get around dma issue.

#endif


    if (len==0)
        return 0;             //no need to send anything

    memset(&t, 0, sizeof(t));       //Zero out the transaction

    t.length   = len*8;                 //Len is in bytes, transaction length is in bits.
    t.rxlength = (len)*8;
    t.tx_buffer = buffer;               //Data
    t.rx_buffer = buffer;               //Data

    int ret = spi_device_polling_transmit(spi, &t);  //Transmit!

    return ret;
}

static int spi__transceive2(spi_device_handle_t spi, uint8_t *tx_buffer, unsigned txlen, uint8_t *rx_buffer, unsigned rxlen)
{
    spi_transaction_t t;

#if 0
#define SPI_TRANS_MODE_DIO            (1<<0)  ///< Transmit/receive data in 2-bit mode
#define SPI_TRANS_MODE_QIO            (1<<1)  ///< Transmit/receive data in 4-bit mode
#define SPI_TRANS_USE_RXDATA          (1<<2)  ///< Receive into rx_data member of spi_transaction_t instead into memory at rx_buffer.
#define SPI_TRANS_USE_TXDATA          (1<<3)  ///< Transmit tx_data member of spi_transaction_t instead of data at tx_buffer. Do not set tx_buffer when using this.
#define SPI_TRANS_MODE_DIOQIO_ADDR    (1<<4)  ///< Also transmit address in mode selected by SPI_MODE_DIO/SPI_MODE_QIO
#define SPI_TRANS_VARIABLE_CMD        (1<<5)  ///< Use the ``command_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.
#define SPI_TRANS_VARIABLE_ADDR       (1<<6)  ///< Use the ``address_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.
#define SPI_TRANS_VARIABLE_DUMMY      (1<<7)  ///< Use the ``dummy_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.

    /**
     * This structure describes one SPI transaction. The descriptor should not be modified until the transaction finishes.
     */
    struct spi_transaction_t {
        uint32_t flags;                 ///< Bitwise OR of SPI_TRANS_* flags
        uint16_t cmd;                   /**< Command data, of which the length is set in the ``command_bits`` of spi_device_interface_config_t.
        *
        *  <b>NOTE: this field, used to be "command" in ESP-IDF 2.1 and before, is re-written to be used in a new way in ESP-IDF 3.0.</b>
        *
        *  Example: write 0x0123 and command_bits=12 to send command 0x12, 0x3_ (in previous version, you may have to write 0x3_12).
        */
        uint64_t addr;                  /**< Address data, of which the length is set in the ``address_bits`` of spi_device_interface_config_t.
        *
        *  <b>NOTE: this field, used to be "address" in ESP-IDF 2.1 and before, is re-written to be used in a new way in ESP-IDF3.0.</b>
        *
        *  Example: write 0x123400 and address_bits=24 to send address of 0x12, 0x34, 0x00 (in previous version, you may have to write 0x12340000).
        */
        size_t length;                  ///< Total data length, in bits
        size_t rxlength;                ///< Total data length received, should be not greater than ``length`` in full-duplex mode (0 defaults this to the value of ``length``).
        void *user;                     ///< User-defined variable. Can be used to store eg transaction ID.
        union {
            const void *tx_buffer;      ///< Pointer to transmit buffer, or NULL for no MOSI phase
            uint8_t tx_data[4];         ///< If SPI_USE_TXDATA is set, data set here is sent directly from this variable.
        };
        union {
            void *rx_buffer;            ///< Pointer to receive buffer, or NULL for no MISO phase. Written by 4 bytes-unit if DMA is used.
            uint8_t rx_data[4];         ///< If SPI_USE_RXDATA is set, data is received directly to this variable
        };
    } ;        //the rx data should start from a 32-bit aligned address to get around dma issue.

#endif


    if (txlen==0)
        return 0;             //no need to send anything

    memset(&t, 0, sizeof(t));       //Zero out the transaction

    t.length = txlen*8;                 //Len is in bytes, transaction length is in bits.
    t.rxlength = (rxlen)*8;
    t.tx_buffer = tx_buffer;               //Data
    t.rx_buffer = rx_buffer;               //Data

    int ret = spi_device_polling_transmit(spi, &t);  //Transmit!

    return ret;
}

static void dump__buffer(const uint8_t *data, unsigned len)
{
    printf("[");
    while (len--) {
        printf(" %02x", *data++);
    }
    printf(" ]");
}

static void fpga__init(spi_device_handle_t spi)
{
    uint8_t idbuf[5];
    idbuf[0] = 0x9F;
    idbuf[1] = 0xAA;
    idbuf[2] = 0x55;
    idbuf[3] = 0xAA;
    idbuf[4] = 0x55;

    int r = spi__transceive(spi,idbuf,5);
    if (r<0) {
        printf("SPI transceive: error %d\r\n", r);
    } else {
        printf("FPGA id: ");
        dump__buffer(&idbuf[1], 4);
        printf("\r\n");
    }
}

static void as_flash__init(spi_device_handle_t spi)
{
    uint8_t idbuf[5];
    idbuf[0] = 0x9F;
    idbuf[1] = 0xAA;
    idbuf[2] = 0x55;
    idbuf[3] = 0xAA;
    idbuf[4] = 0x55;

    int r = spi__transceive(spi,idbuf,5);
    if (r<0) {
        printf("SPI transceive: error %d\r\n", r);
    } else {
        printf("SPI Flash id: ");
        dump__buffer(&idbuf[1], 4);
        printf("\r\n");
    }
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

static unsigned int spi_read_id()
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
    unsigned flash_id = spi_read_id();
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

    printf("HDLC: command 0x%02x\r\n", pos);

    if (pos>BOOTLOADER_MAX_CMD)
        return;
    pos--;

    uint8_t *end = handlers[pos](&enc, hdlc_txbuf, data);
    if (end!=NULL) {
        if (end!=hdlc_txbuf) {
            printf("HDLC: Sending reply size %d\n", end - hdlc_txbuf);
            send(sock, hdlc_txbuf, end - hdlc_txbuf, 0);
        }
    }
}




static void cmd_server_task(void *pvParameters)
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
                    /*
                     // Get the sender's ip address as string
                     if (source_addr.sin6_family == PF_INET) {
                     inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                     } else if (source_addr.sin6_family == PF_INET6) {
                     inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                     }

                     rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                     ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                     ESP_LOGI(TAG, "%s", rx_buffer);

                     int err = send(sock, rx_buffer, len, 0);
                     */
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




void app_main()
{
    gpio__init();
    led__set(LED1, 1);
    spi__init(&spi0_fpga, &spi0_as_flash);
    led__set(LED2, 1);
    sdcard__init();
    wifi__init();

    fpga__init(spi0_fpga);
    as_flash__init(spi0_as_flash);

    xTaskCreate(cmd_server_task, "cmd_server", 4096, NULL, 5, NULL);

    while(1) {
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}
