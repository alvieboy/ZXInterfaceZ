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
#include "fpga.h"
#include "flash_pgm.h"
#include "dump.h"
#include "command.h"
#include "ota.h"
#include "fpga_ota.h"
#include "sna.h"
#include "res.h"
#include "strtoint.h"
#include "netcomms.h"

static xQueueHandle gpio_evt_queue = NULL;

static volatile uint32_t interrupt_count = 0;


static uint8_t fb[16384];



#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

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
            .channel = 3,
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
#ifdef SDMMC_PIN_DET
    slot_config.gpio_cd = SDMMC_PIN_DET;
#endif
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

    gpio_set_level(PIN_NUM_NCONFIG, 1);

    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT_OD;
    io_conf.pin_bit_mask = (1ULL<<PIN_NUM_LED1) | (1ULL<<PIN_NUM_LED2)
        | (1ULL<<PIN_NUM_NCONFIG);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en   = 0;

    gpio_config(&io_conf);


    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        (1ULL<<PIN_NUM_AS_CSN) |
        (1ULL<<PIN_NUM_CS);

    gpio_config(&io_conf);

    gpio_set_level(PIN_NUM_AS_CSN, 1);
    gpio_set_level(PIN_NUM_CS, 1);


    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
        (1ULL<<PIN_NUM_SWITCH) |
        (1ULL<<PIN_NUM_CONF_DONE) |
        (1ULL<<PIN_NUM_IO25) |
        (1ULL<<PIN_NUM_SPECT_INTERRUPT) |
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

static int send_framebuffer(command_t *cmdt, int argc, char **argv)
{
    fpga__get_framebuffer(fb);

    ESP_LOGI(TAG, "Sending framebuffer");
    int len = send(cmdt->socket, &fb[4], SPECTRUM_FRAME_SIZE, 0);


    if (len != SPECTRUM_FRAME_SIZE) {
        ESP_LOGE(TAG, "send failed: errno %d", errno);
        return COMMAND_CLOSE_SILENT;
    }

    return COMMAND_CLOSE_SILENT;
}

// Target needs extra 4 bytes at start



volatile int client_socket = -1;

#define MAX_FRAME_PAYLOAD 2048

struct frame {
    uint8_t seq;
    uint8_t frag;
    uint8_t payload[MAX_FRAME_PAYLOAD];
};

static struct frame f; // ensure we have stack!!!

static int fill_and_send_frames(int client_socket,
                                uint8_t seq,
                                const uint8_t *data)
{
    f.seq = seq;

    unsigned off = 0;
    uint8_t frag = 0;
    unsigned size = SPECTRUM_FRAME_SIZE;
    while (size) {
        unsigned chunk = size > MAX_FRAME_PAYLOAD?MAX_FRAME_PAYLOAD:size;
        f.frag = frag;

        //ESP_LOGI(TAG, "Chunk %d off %d frag %d", chunk, off, frag);

        memcpy(f.payload, &data[off], chunk);

        off+=chunk;
        size-=chunk;
        chunk += 2; // Size of header

        if (send( client_socket, &f, chunk, 0)!=chunk) {
            return -1;
        }
        frag++;
    }
    return 0;
}

static void streamer_server_task(void *pvParameters)
{
    uint32_t io_num;
    uint8_t seqno = 0;
    do {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            fpga__get_framebuffer(fb);
            if (client_socket>0) {
                //ESP_LOGI(TAG, "Sending frames sock %d", client_socket);
                if (fill_and_send_frames( client_socket, seqno++, &fb[4])<0) {
                    ESP_LOGE(TAG, "Error sending to client socket");
                    shutdown(client_socket,0);
                    close(client_socket);
                    client_socket = -1;
                }
            }
            seqno++;
        }
    } while (1);
}

static int do_reset_spectrum(command_t *cmdt, int argc, char **argv, bool forcerom)
{
    bool do_cap = false;
    bool has_mask = false;
    bool has_value = false;

    uint32_t mask;
    uint32_t value;

    int argindex=0;
    ESP_LOGI(TAG, "Argc %d\n", argc);
    // Check if we have capture
    if (argc>0) {
        if (strcmp(argv[argindex], "cap")==0) {
            argindex++;
            argc--;
            do_cap=true;
        }
    }
    if (do_cap && argc>0) {
        // Mask
        ESP_LOGI(TAG, "Has mask");
        if (strtoint(argv[argindex], (int*)&mask)==0) {
            has_mask=true;
            argindex++;
            argc--;
        } else {
            return -1;
        }
    }

    if (has_mask && argc>0) {
        // Mask
        ESP_LOGI(TAG, "Has value");
        if (strtoint(argv[argindex], (int*)&value)==0) {
            has_value=true;
            argindex++;
            argc--;
        } else {
            return -1;
        }
    }

    if (forcerom)
        ESP_LOGI(TAG, "Resetting spectrum (to custom ROM)");
    else
        ESP_LOGI(TAG, "Resetting spectrum (to internal ROM)");


    if (has_mask) {
        ESP_LOGI(TAG, "Enabling capture mask 0x%08x", mask);
        fpga__set_capture_mask(mask);
    }

    if (has_value) {
        ESP_LOGI(TAG, "Enabling capture value 0x%08x", value);
        fpga__set_capture_value(value);
    }
    if (do_cap) {

        fpga__set_flags(FPGA_FLAG_RSTSPECT | FPGA_FLAG_CAPCLR );

        if (forcerom)
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_ON);
        else
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_OFF);

        vTaskDelay(2 / portTICK_RATE_MS);
        fpga__set_clear_flags(FPGA_FLAG_CAPRUN, FPGA_FLAG_CAPCLR | FPGA_FLAG_RSTSPECT | FPGA_FLAG_COMPRESS);

    } else {
        fpga__set_flags(FPGA_FLAG_RSTSPECT | FPGA_FLAG_CAPCLR);
        if (forcerom)
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_ON);
        else
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_OFF);

        vTaskDelay(2 / portTICK_RATE_MS);
        fpga__clear_flags(FPGA_FLAG_RSTSPECT);
    }
    ESP_LOGI(TAG, "Reset completed");
    return 0;
}

static int reset_spectrum(command_t *cmdt, int argc, char **argv)
{
    return do_reset_spectrum(cmdt, argc, argv, 0);
}

static int reset_custom_spectrum(command_t *cmdt, int argc, char **argv)
{
    return do_reset_spectrum(cmdt, argc, argv, 1);
}



static int send_captures(command_t *cmdt, int argc, char **argv)
{
    // Get capture flags

    uint32_t flags = fpga__get_capture_status();
    ESP_LOGI(TAG,"Capture flags: 0x%08x", flags);

    int nlen = fpga__get_captures(fb);

    const uint8_t *txptr = &fb[2];

    nlen -= 2;

    // Find limit.

    int len;

    do {
        len = send(cmdt->socket, txptr, nlen, 0);
        if (len>0) {
            nlen -= len;
        } else {
            break;
        }
    } while (nlen>0);

    if (len < 0) {
        ESP_LOGE(TAG, "send failed: errno %d", errno);
        return -1;
    }
    return 0;
}


int strtoint(const char *str, int *dest)
{
    char *endptr;
    int val = strtoul(str,&endptr, 0);
    if (endptr) {
        if (*endptr=='\0') {
            *dest = val;
            return 0;
        }
    }
    return -1;
}

int start_stream(command_t *cmdt, int argc, char **argv)
{
    int port;
    struct sockaddr_in s;
    int r = -1;

    if (cmdt->source_addr->sin_family!=AF_INET)
        return r;

    if (argc<1)
        return r;

    do {
        if (strtoint(argv[0], &port)<0) {
            break;
        }

        // create socket.
        int tsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (tsock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        s.sin_family = AF_INET;
        s.sin_port = htons(port);

        s.sin_addr = cmdt->source_addr->sin_addr;

        int err = connect(tsock, (struct sockaddr *)&s, sizeof(s));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }

        ESP_LOGI(TAG, "Socket created, %s:%d", inet_ntoa(s.sin_addr.s_addr), (int)port);
        client_socket = tsock;
        r = 0;

    } while (0);

    return r;
}

static int upload_rom_data(command_t *cmdt)
{
    unsigned remain = cmdt->romsize - cmdt->romoffset;

    if (remain<cmdt->len) {
        // Too much data, complain
        ESP_LOGE(TAG, "ROM: expected max %d but got %d bytes", remain, cmdt->len);
        return -1;
    }

    ESP_LOGI(TAG, "ROM: offset %d before uploading %d bytes", cmdt->romoffset, cmdt->len);

    cmdt->romoffset += fpga__upload_rom_chunk(cmdt->romoffset, &cmdt->tx_prebuffer[1], cmdt->len);

    /*
    // Upload chunk
    cmdt->tx_prebuffer[1] = 0xE1;
    cmdt->tx_prebuffer[2] = (cmdt->romoffset>>8) & 0xFF;
    cmdt->tx_prebuffer[3] = cmdt->romoffset & 0xFF;

    {
        dump__buffer(&cmdt->tx_prebuffer[1], 8);
    }

    spi__transceive(spi0_fpga, &cmdt->tx_prebuffer[1], 3 + cmdt->len);

    cmdt->romoffset += cmdt->len;
    */
    ESP_LOGI(TAG, "ROM: offset %d after uploading %d bytes", cmdt->romoffset, cmdt->len);

    cmdt->len = 0; // Reset receive ptr.

    remain = cmdt->romsize - cmdt->romoffset;

    if (remain==0)
        return 0;

    return 1;
}

static int upload_rom(command_t *cmdt, int argc, char **argv)
{
    int romsize;

    if (argc<1)
        return -1;

    // Extract size from params.
    if (strtoint(argv[0], &romsize)<0) {
        return -1;
    }
    if (romsize > 16384)
        return -1;

    cmdt->romsize = romsize;
    cmdt->romoffset = 0;
    cmdt->rxdatafunc = &upload_rom_data;
    cmdt->state = READDATA;

    ESP_LOGI(TAG, "Uploading ROM requested size %d\n", romsize);

    return 1; // Continue receiving data.
}

static int scap(command_t *cmdt, int argc, char **argv)
{
    if (argc>0) {
        ESP_LOGI(TAG, "Putting spectrum under RESET");
        fpga__set_flags(FPGA_FLAG_RSTSPECT);
    }

    ESP_LOGE(TAG, "Capture mask: %08x",fpga__get_register( REG_CAPTURE_MASK ));
    ESP_LOGE(TAG, "Capture value: %08x",fpga__get_register( REG_CAPTURE_VAL ));
    ESP_LOGE(TAG, "r2: %08x", fpga__get_register( 2 ));
    ESP_LOGE(TAG, "r3: %08x", fpga__get_register( 3 ));
    //set_capture_mask(0x0);
    //set_capture_value(0x0);

    fpga__clear_flags(FPGA_FLAG_CAPRUN | FPGA_FLAG_COMPRESS);
    fpga__set_flags(FPGA_FLAG_CAPCLR);
    fpga__set_clear_flags(FPGA_FLAG_CAPRUN, FPGA_FLAG_CAPCLR);
    ESP_LOGI(TAG, "Starting forced capture");

    vTaskDelay(500 / portTICK_RATE_MS);

    uint32_t stat = fpga__get_capture_status();
    ESP_LOGI(TAG, "Capture status: %08x\n", stat);

    if (argc>0) {
        ESP_LOGI(TAG, "Releasing spectrum from RESET");
        fpga__clear_flags(FPGA_FLAG_RSTSPECT);
    }
    return 0;
}

struct commandhandler_t hand[] = {
    { CMD("fb"), &send_framebuffer },
    { CMD("cap"), &send_captures },
    { CMD("stream"), &start_stream },
    { CMD("uploadrom"), &upload_rom },
    { CMD("reset"), &reset_spectrum },
    { CMD("scap"), &scap },
    { CMD("resettocustom"), &reset_custom_spectrum },
    { CMD("ota"), &ota__performota },
    { CMD("fpgaota"), &fpga_ota__performota },
    { CMD("uploadsna"), &sna__uploadsna },
    { CMD("uploadres"), &res__upload },
};


static int check_command(command_t *cmdt, uint8_t *nl)
{
    int i;
    struct commandhandler_t *h;
    int r;
    // Terminate it.
    *nl++ = '\0';

#define MAX_TOKS 8

    char *toks[MAX_TOKS];
    int tokidx = 0;
    char *p = (char*)&cmdt->rx_buffer[0];

    while ((toks[tokidx] = strtok_r(p, " ", &p))) {
        tokidx++;
        if (tokidx==MAX_TOKS) {
            return -1;
        }
    }

    int thiscommandlen = strlen(toks[0]);


    for (i=0; i<sizeof(hand)/sizeof(hand[0]); i++) {
        h = &hand[i];
        if (h->cmdlen == thiscommandlen) {
            if (strcmp(h->cmd, toks[0])==0) {
                r = h->handler( cmdt, tokidx-1, &toks[1] );

                // TBD: copy extra data.
                //ESP_LOGI(TAG, "NL disparity %d len is %d\n", nl - (&cmdt->rx_buffer[0]), cmdt->len);
                int remain  = cmdt->len - (nl - (&cmdt->rx_buffer[0]));
                if (remain>0) {
                    memmove(&cmdt->rx_buffer[0], nl, remain);
                    cmdt->len = remain;
                } else {
                    // "Eat"
                    cmdt->len = 0;
                }
                return r;
            }
        }
    }
    ESP_LOGE(TAG,"Could not find a command handler for %s", toks[0]);

#if 0
    if (strncmp(rx, "fb", 2)==0) {
        send_framebuffer( sock );
    } else if (strncmp(rx, "cap", 3)==0) {
        send_captures( sock );
    } else if (strncmp(rx, "stream ",7)==0) {
        start_stream((struct sockaddr_in*)&source_addr, &rx[7]);
    } else if (strncmp(rx, "uploadrom ",10)==0) {
        upload_rom((struct sockaddr_in*)&source_addr, &rx[10], len-10);
    }
#endif
    return -1;
}

static command_result_t handle_command(command_t *cmdt)
{
    uint8_t *nl = NULL;
    int r;

    do {
        int len = recv(cmdt->socket,
                       &cmdt->rx_buffer[cmdt->len],
                       sizeof(cmdt->rx_buffer) - (cmdt->len),
                       0);

        if (len<0) {
            return COMMAND_CLOSE_SILENT;
        }

        cmdt->len += len;

        switch (cmdt->state) {
        case READCMD:
            // Looking for newline. NEEDS to be '\n';
            nl = memchr( cmdt->rx_buffer, '\n', cmdt->len);
            if (!nl) {
                // So far no newline.
                if (cmdt->len >= sizeof(cmdt->rx_buffer)) {
                    // Overflow, we cannot store more data.
                    return COMMAND_CLOSE_ERROR;
                }
                // Just continue to get more data.
            } else {
                r = check_command(cmdt, nl);
                if (r != COMMAND_CONTINUE)
                    return r;
                // Now, if we moved to READDATA and we still have data, process it
                if (cmdt->state == READDATA && (cmdt->len>0)) {
                    r = cmdt->rxdatafunc(cmdt);
                    if (r != COMMAND_CONTINUE)
                        return r;
                }
            }
            break;
        case READDATA:
            r = cmdt->rxdatafunc(cmdt);

            if (r != COMMAND_CONTINUE)
                return r;
            break;
        }
    } while (1);

    return COMMAND_CLOSE_OK;
}


static void buffer_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    
    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(BUFFER_PORT);
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
            command_t cmdt;


            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                break;
            }

            ESP_LOGI(TAG, "Socket accepted");

            cmdt.socket = sock;
            cmdt.source_addr = (struct sockaddr_in*)&source_addr;
            cmdt.len = 0;
            cmdt.state = READCMD;
            cmdt.errstr = NULL;

            command_result_t r = handle_command(&cmdt);
            ESP_LOGI(TAG, "Command result: %d", r);
            switch (r) {
            case COMMAND_CLOSE_OK:
                netcomms__send_ok(sock);
                break;
            case COMMAND_CLOSE_ERROR:
                netcomms__send_error(sock, cmdt.errstr);
                break;
            default:
                break;
            }

            shutdown(sock,3);
            close(sock);
            sock = -1;
        }
    }
    vTaskDelete(NULL);
}


void start_capture2()
{
    // Stop, clear
    fpga__set_clear_flags( FPGA_FLAG_CAPCLR, FPGA_FLAG_CAPRUN);
    fpga__set_flags( FPGA_FLAG_CAPRUN);
}

void start_capture()
{
    // Reset spectrum, stop capture
    fpga__set_clear_flags( FPGA_FLAG_RSTSPECT, FPGA_FLAG_CAPRUN );
    // Reset fifo, clear capture
    fpga__set_flags( FPGA_FLAG_CAPCLR | FPGA_FLAG_RSTFIFO);

    // Remove resets, start capture
    fpga__set_clear_flags( FPGA_FLAG_CAPRUN | FPGA_FLAG_ENABLE_INTERRUPT , FPGA_FLAG_RSTFIFO| FPGA_FLAG_RSTSPECT| FPGA_FLAG_CAPCLR );

}


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    interrupt_count++;
}

static void specinterrupt__init()
{
    gpio_evt_queue = xQueueCreate(8, sizeof(uint32_t));

    xTaskCreate(streamer_server_task, "streamer_task", 4096, NULL, 10, NULL);

    gpio_set_intr_type(PIN_NUM_SPECT_INTERRUPT, GPIO_INTR_NEGEDGE);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(PIN_NUM_SPECT_INTERRUPT, gpio_isr_handler, (void*) PIN_NUM_SPECT_INTERRUPT);
}

volatile int restart_requested = 0;

void request_restart()
{
    restart_requested = 1;
}

void app_main()
{
    gpio__init();
    led__set(LED1, 1);
    spi__init_bus();

    led__set(LED2, 1);
//    sdcard__init();
    wifi__init();

    fpga__init();
    flash_pgm__init();

    xTaskCreate(buffer_server_task, "buffer_server", 4096, NULL, 5, NULL);

    int lstatus = 0;

    specinterrupt__init();

    // Start capture
    start_capture();
    int lastsw=1;
    int do_restart = 0;

    while(1) {
        //int conf = gpio_get_level(PIN_NUM_CONF_DONE);
//        ESP_LOGI(TAG,"CONF_DONE pin: %d\r\n", conf);
        led__set(LED1, lstatus);
        lstatus = !lstatus;
        int sw = gpio_get_level(PIN_NUM_SWITCH);
        if (sw==0 && lastsw==1) {
            ESP_LOGI(TAG, "Start capture");
            start_capture2();
        }
        lastsw = sw;

        if (restart_requested)
            do_restart = 1;

        vTaskDelay(1000 / portTICK_RATE_MS);
        //ESP_LOGI(TAG,"Interrupts: %d\n", interrupt_count);
        if (do_restart)
            esp_restart();

    }
}
