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

char cmd[256];
uint8_t cmdptr = 0;

#define CTAG "CONSOLE"

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

    audio__set_volume_f(chan, (float)volume/100.0F,
                        -1.0 + ((float)balance/50.0));
    return 0;
}

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
    } else {
        ESP_LOGE(CTAG, "Unrecognised USB command %s", argv[0]);
    }
    return 0;
}

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
        return wifi__config_ap(ssid, password, channel);

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


static int console__loadrom(int argc, char **argv)
{
    if (argc<1) {
        ESP_LOGE(CTAG, "Too few arguments");
        return -1;
    }
    int f = rom__load_custom_from_file(argv[0]);
    if (f<0) {
        ESP_LOGE(CTAG, "Cannot load ROM '%s'", argv[0]);
        return -1;
    }

    if (fpga__reset_to_custom_rom(false)<0) {
        ESP_LOGE(CTAG, "Cannot reset");
        return -1;
    }
    return 0;
}

static int console__reset(int argc, char **argv)
{
    fpga__reset_spectrum();
    return 0;
}

static struct {
    const char *cmd;
    int (*handler)(int, char**);
} hand[] = {
    { "vol", &console__volume },
    { "usb", &console__usb },
    { "ulahack", &console__ulahack },
    { "wifi", &console__wifi },
    { "loadrom", &console__loadrom },
    { "reset", &console__reset },
};

static int console__parse_string(char *cmd);

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

    int i;

    for (i=0; i<sizeof(hand)/sizeof(hand[0]); i++) {
        if (strcmp(hand[i].cmd, toks[0])==0) {
            int r = hand[i].handler(tokidx-1, &toks[1] );
            return r;
        }
    }
    ESP_LOGE(CTAG,"Invalid command %s", toks[0]);
    return 0;
}
