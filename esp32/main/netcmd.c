#include <string.h>
#include "netcmd.h"
#include "command.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "defs.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "fpga.h"
#include "netcomms.h"
#include "videostreamer.h"
#include "strtoint.h"
#include "res.h"
#include "ota.h"
#include "sna.h"
#include "fpga_ota.h"



#ifdef __linux__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static inline char* inet_ntoa_r(struct in_addr in, char *dest, unsigned len)
{
    char *n  = inet_ntoa(in);
    strncpy(dest, n, len);
    return dest;
}

#endif


static int netcmd__send_framebuffer(command_t *cmdt, int argc, char **argv);
static int netcmd__send_captures(command_t *cmdt, int argc, char **argv);

static int netcmd__start_stream(command_t *cmdt, int argc, char **argv)
{
    int port;

    if(cmdt->source_addr->sin_family!=AF_INET)
        return -1;

    if (strtoint(argv[0], &port)<0) {
        return -1;
    }

    return videostreamer__start_stream(cmdt->source_addr->sin_addr, port );
}


static int netcmd__send_captures(command_t *cmdt, int argc, char **argv)
{
    // Get capture flags
#if 0
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
#endif
    return -1;
}
    /*
static int netcmd__scap(command_t *cmdt, int argc, char **argv)
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
}     */

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

static int netcmd__upload_rom(command_t *cmdt, int argc, char **argv)
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


    /*if (has_mask) {
        ESP_LOGI(TAG, "Enabling capture mask 0x%08x", mask);
        fpga__set_capture_mask(mask);
    }

    if (has_value) {
        ESP_LOGI(TAG, "Enabling capture value 0x%08x", value);
        fpga__set_capture_value(value);
    } */
    if (do_cap) {

        fpga__set_flags(FPGA_FLAG_RSTSPECT | FPGA_FLAG_CAPCLR );

        if (forcerom)
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_ON);
        else
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_OFF);

        vTaskDelay(2 / portTICK_RATE_MS);
        fpga__set_clear_flags(FPGA_FLAG_CAPRUN, FPGA_FLAG_CAPCLR | FPGA_FLAG_RSTSPECT | FPGA_FLAG_COMPRESS);

    } else {
        fpga__set_clear_flags(FPGA_FLAG_RSTSPECT | FPGA_FLAG_CAPCLR, FPGA_FLAG_TRIG_FORCEROMONRETN);
        if (forcerom)
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_ON);
        else
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_OFF);

        vTaskDelay(2 / portTICK_RATE_MS);
        fpga__clear_flags(FPGA_FLAG_RSTSPECT);
    }
    fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_OFF);
    ESP_LOGI(TAG, "Reset completed");
    return 0;
}


static int netcmd__reset_spectrum(command_t *cmdt, int argc, char **argv)
{
    return do_reset_spectrum(cmdt, argc, argv, 0);
}

static int netcmd__reset_custom_spectrum(command_t *cmdt, int argc, char **argv)
{
    return do_reset_spectrum(cmdt, argc, argv, 1);
}


static int netcmd__vgamode(command_t *cmdt, int argc, char **argv)
{
    //int mode;
    /*
    if (strtoint(argv[0], &mode)<0) {
        return -1;
    }
    if (mode) {
        fpga__set_flags(FPGA_FLAG_VIDMODEWIDE);
    }
    else {
        fpga__clear_flags(FPGA_FLAG_VIDMODEWIDE);
    } */
    return 0;
}


struct commandhandler_t hand[] = {
    { CMD("fb"), &netcmd__send_framebuffer },
    { CMD("cap"), &netcmd__send_captures },
    { CMD("stream"), &netcmd__start_stream },
    { CMD("uploadrom"), &netcmd__upload_rom },
    { CMD("reset"), &netcmd__reset_spectrum },
//    { CMD("scap"), &netcmd__scap },
    { CMD("resettocustom"), &netcmd__reset_custom_spectrum },
    { CMD("ota"), &ota__performota_cmd },
    { CMD("fpgaota"), &fpga_ota__performota },
    { CMD("uploadsna"), &sna__uploadsna },
    { CMD("uploadres"), &res__upload },
    { CMD("vgamode"), &netcmd__vgamode },
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

static int netcmd__send_framebuffer(command_t *cmdt, int argc, char **argv)
{
    const uint8_t *fb = videostreamer__getlastfb();

    ESP_LOGI(TAG, "Sending framebuffer");
    int len = send(cmdt->socket, &fb[4], SPECTRUM_FRAME_SIZE, 0);


    if (len != SPECTRUM_FRAME_SIZE) {
        ESP_LOGE(TAG, "send failed: errno %d", errno);
        return COMMAND_CLOSE_SILENT;
    }

    return COMMAND_CLOSE_SILENT;
}

static void netcmd__server_task(void *pvParameters)
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

void netcmd__init()
{
    xTaskCreate(netcmd__server_task, "buffer_server", 4096, NULL, 5, NULL);
}





