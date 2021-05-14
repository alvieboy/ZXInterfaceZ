/**
 * \defgroup console Console commands
 * \brief Console command handler
 */
#include "console.h"
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "defs.h"
#include "strtoint.h"
#include "audio.h"
#include "usbh.h"
#include "fpga.h"
#include "wifi.h"
#include "rom.h"
#include "memlayout.h"
#include "log.h"
#include "hdlc_encoder.h"
#include "version.h"
#include "esp32/rom/uart.h"
#include "byteops.h"
#include "scope.h"
#include "reset.h"
#include "rom_hook.h"

#define TAG "CONSOLE"

static char cmd[256];
static uint8_t cmdptr = 0;
static hdlc_encoder_t enc;

#define CTAG "CONSOLE"


static void console__hdlc_write(void *user, uint8_t val)
{
    while (uart_tx_one_char(val)!=0) {
        vTaskDelay(20 / portTICK_RATE_MS);
    }
}
/**
 * \ingroup console
 * \brief Initialise the console subsystem
 */
void console__init(void)
{
    hdlc_encoder__init(&enc, &console__hdlc_write, NULL, NULL);
}

static void console__send_scope_group(hdlc_encoder_t *enc, scope_group_t group)
{
    uint8_t len = (uint8_t)scope__get_group_size(group);
    int i;

    hdlc_encoder__write(enc, &len, 1);

    for (i=0; i<(int)len;i++) {
        const char *name = scope__get_signal_name(group, i);
        if (name) {
            hdlc_encoder__write(enc, name, strlen(name)+1);  // Include NULL
        } else {
            // Should not happen!
            ESP_LOGE(TAG, "Invalid signal name!!!");
        }
    }
}

void console__hdlc_data(const uint8_t *d, unsigned len)
{
    uint8_t reply;

    if (len<1)
        return;

    if (d[0]==0x01) {
        reply = 0x81;
        // Version and info
        hdlc_encoder__begin(&enc);
        hdlc_encoder__write(&enc, &reply,1);
        hdlc_encoder__write(&enc, version, strlen(version)+1);  // Include NULL at end
        console__send_scope_group(&enc, SCOPE_GROUP_NONTRIG);
        console__send_scope_group(&enc, SCOPE_GROUP_TRIG);
        hdlc_encoder__end(&enc);
    }
    if (d[0]==0x02 && len==13) {
        uint32_t mask = extractle32(&d[1]);
        uint32_t val = extractle32(&d[5]);
        uint32_t edge = extractle32(&d[9]);

        scope__set_triggers(mask,val,edge);

        reply = 0x82;
        hdlc_encoder__begin(&enc);
        hdlc_encoder__write(&enc, &reply,1);
        hdlc_encoder__end(&enc);
    }

    if (d[0]==0x03 && len==2) {
        scope__start(d[1]);

        reply = 0x83;
        hdlc_encoder__begin(&enc);
        hdlc_encoder__write(&enc, &reply,1);
        hdlc_encoder__end(&enc);
    }
    if (d[0]==0x04) {
        union {
            struct {
                uint32_t status;
                uint32_t counter;
                uint32_t trig_address;
            };
            uint8_t buf[12];
        } data;

        scope__get_capture_info(&data.status, &data.counter, &data.trig_address);

        reply = 0x84;

        hdlc_encoder__begin(&enc);
        hdlc_encoder__write(&enc, &reply,1);
        hdlc_encoder__write(&enc, &data.buf[0], 12);
        hdlc_encoder__end(&enc);
    }
    if (d[0]==0x05) {
        // d[1] holds ram and offset;
        uint8_t channel = d[1] & 0x80 ? 1: 0;
        uint8_t offset = d[1] & 0x0F;
        union {
            uint8_t w8[256];
            uint32_t w32[64];
        } buf;

        scope__get_capture_data_block64(channel, offset, &buf.w32[0]);

        reply = 0x85;

        hdlc_encoder__begin(&enc);
        hdlc_encoder__write(&enc, &reply,1);
        hdlc_encoder__write(&enc, &d[2],1); // Sequence
        hdlc_encoder__write(&enc, &buf.w8[0], 256);
        hdlc_encoder__end(&enc);

    }

}

/**
 * \ingroup console
 * \brief Handle console command "volume"
 */
static int console__volume(int argc, char **argv)
{
    int chan;
    int volume;
    int balance;

    if (argc<3) {
        ESP_LOGE(CTAG, "Too few arguments");
        return -1;
    }

    if (strtoint(argv[0], &chan)<0) {
        ESP_LOGE(CTAG, "Channel not integer!");
        return -1;
    }
    if (chan<0 || chan>3) {
        ESP_LOGE(CTAG, "Invalid channel %d!", chan);
        return -1;
    }

    if (strtoint(argv[1], &volume)<0) {
        ESP_LOGE(CTAG, "Volume not integer!");
        return -1;
    }
    if (volume<0 || volume>100) {
        ESP_LOGE(CTAG, "Invalid volume %d!", volume);
        return -1;
    }

    if (strtoint(argv[2], &balance)<0) {
        ESP_LOGE(CTAG, "Volume not integer!");
        return -1;
    }
    if (balance<0 || balance>100) {
        ESP_LOGE(CTAG, "Invalid balance %d!", balance);
        return -1;
    }

    audio__set_volume_f((uint8_t)chan, (float)volume/100.0F,
                        -1.0F + (float)balance/50.0F);
    return 0;
}


/**
 * \ingroup console
 * \brief Handle console command "usb"
 */
static int console__usb(int argc, char **argv)
{
    if (argc<1) {
        ESP_LOGE(CTAG, "Too few arguments");
        return -1;
    }
    if (strcmp(argv[0],"on")==0) {
        ESP_LOGI(CTAG, "Powering ON USB");
        usb_ll__set_power(1);
    } else if (strcmp(argv[0],"off")==0) {
        ESP_LOGI(CTAG, "Powering OFF USB");
        usb_ll__set_power(0);
    } else if (strcmp(argv[0],"info")==0) {
        usbh__dump_info();
    } else {
        ESP_LOGE(CTAG, "Unrecognised USB command %s", argv[0]);
    }
    return 0;
}

/**
 * \ingroup console
 * \brief Handle console command "ulahack"
 */
static int console__ulahack(int argc, char **argv)
{
    if (argc<1) {
        ESP_LOGE(CTAG, "Too few arguments");
        return -1;
    }
    if (strcmp(argv[0],"on")==0) {
        ESP_LOGI(TAG, "Enable ULAHACK");
        fpga__set_flags(FPGA_FLAG_ULAHACK);
    } else if (strcmp(argv[0],"off")==0) {
        ESP_LOGI(TAG, "Disable ULAHACK");
        fpga__clear_flags(FPGA_FLAG_ULAHACK);
    } else {
        ESP_LOGE(CTAG, "Unrecognised ULAHACK command %s", argv[0]);
    }
    return 0;
}

/**
 * \ingroup console
 * \brief Handle console command "wifi"
 */
static int console__wifi(int argc, char **argv)
{
    const char *ssid = NULL;
    const char *password = NULL;
    int channel = 0;

    if (argc<1) {
        ESP_LOGE(CTAG, "Too few arguments");
        return -1;
    }
    if (strcmp(argv[0],"ap")==0) {
        if (argc>1) {
            // Get ssid
            ssid = argv[1];
            if (argc>2) {
                password = argv[2];
                if (argc>3) {
                    if ((strtoint(argv[3], &channel)<0) || (channel<0) || (channel>255)) {
                        ESP_LOGE(CTAG,"Invalid channel specified");
                        return -1;
                    }
                }
            }
        }
        return wifi__config_ap(ssid, password, (uint8_t)channel);

    } else if (strcmp(argv[0],"sta")==0) {
        if (argc<3) {
            ESP_LOGE(CTAG, "Too few arguments");
            return -1;
        }
        ssid = argv[1];
        password = argv[2];

        return wifi__config_sta(ssid, password);

    } else {
        ESP_LOGE(CTAG, "Unrecognised WiFi command %s", argv[0]);
    }
    return 0;
}


/**
 * \ingroup console
 * \brief Handle console command "loadrom"
 */
static int console__loadrom(int argc, char **argv)
{
    if (argc<1) {
        ESP_LOGE(CTAG, "Too few arguments");
        return -1;
    }
    int f = rom__load_custom_from_file(argv[0], MEMLAYOUT_ROM2_BASEADDRESS);
    if (f<0) {
        ESP_LOGE(CTAG, "Cannot load ROM '%s'", argv[0]);
        return -1;
    }

    if (reset__reset_to_custom_rom(ROM_2, 0x00, false)<0) {
        ESP_LOGE(CTAG, "Cannot reset");
        return -1;
    }
    return 0;
}

/**
 * \ingroup console
 * \brief Handle console command "reset"
 */
static int console__reset(int argc, char **argv)
{
    if (argc>0) {
        if (strcmp(argv[0], "custom")==0) {
            ESP_LOGI(CTAG, "Resetting to custom ROM0");
            reset__reset_to_custom_rom(ROM_0, 0x00, false);
            return 0;
        }
    }
    reset__reset_spectrum();
    return 0;
}

/**
 * \ingroup console
 * \brief Handle console command "debug"
 */
static int console__debug(int argc, char **argv)
{
    ESP_LOGI(CTAG, "Enabling ALL debug");
    loglevel = 0xffffffff;
    return 0;
}

/**
 * \ingroup console
 * \brief Handle console command "info"
 */
static int console__info(int argc, char **argv)
{
    ESP_LOGI(CTAG, "Memory hook configuration: ");
    rom_hook__dump();
    return 0;
}

static struct {
    const char *cmd;
    int (*handler)(int, char**);
    const char *help;
} command_handlers[] = {
    { "vol", &console__volume , "Set audio volume"},
    { "usb", &console__usb, "USB commands"},
    { "ulahack", &console__ulahack, "ULA hack settings"},
    { "wifi", &console__wifi, "WiFi configutation"},
    { "loadrom", &console__loadrom, "ROM loading" },
    { "reset", &console__reset, "Spectrum reset" },
    { "debug", &console__debug, "Debug settings" },
    { "info", &console__info, "Show miscelaneous info" }
};

static int console__parse_string(char *cmd);

/**
 * \ingroup console
 * \brief Handle a console char
 */
void console__char(char c)
{
    if (c==0x0d || c==0x0a) {
        cmd[cmdptr] = '\0';
        if (cmdptr>0) {
            console__parse_string(cmd);
        }
        cmdptr = 0;
    } else if (c==0x08) {
        if (cmdptr>0) {
            cmdptr--;
        }
    } else {
        cmd[cmdptr++] = c;
    }
}

/**
 * \ingroup console
 * \brief Parse and execute a console line 
 */
static int console__parse_string(char *cmd)
{
#define MAX_TOKS 8
    char *toks[MAX_TOKS];
    int tokidx = 0;
    char *p = cmd;

    while ((toks[tokidx] = strtok_r(p, " ", &p))) {
        tokidx++;
        if (tokidx==MAX_TOKS) {
            return -1;
        }
    }

    unsigned i;

    for (i=0; i<sizeof(command_handlers)/sizeof(command_handlers[0]); i++) {
        if (strcmp(command_handlers[i].cmd, toks[0])==0) {
            int r = command_handlers[i].handler(tokidx-1, &toks[1] );
            return r;
        }
    }
    ESP_LOGE(CTAG,"Invalid command %s", toks[0]);
    return 0;
}
