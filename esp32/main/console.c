#include "console.h"
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "defs.h"
#include "strtoint.h"
#include "audio.h"
#include "usbh.h"
#include "fpga.h"

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

static struct {
    const char *cmd;
    int (*handler)(int, char**);
} hand[] = {
    { "vol", &console__volume },
    { "usb", &console__usb },
    { "ulahack", &console__ulahack },
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

#if 0
    if (c=='p') {
        ESP_LOGI(TAG, "Powering ON USB");
        usb_ll__set_power(1);
    } else if (c=='o') {
        ESP_LOGI(TAG, "Powering OFF USB");
        usb_ll__set_power(0);
    } else if (c=='f') {
        ESP_LOGI(TAG, "Dumping information");
        usbh__dump_info(0);
    } else if (c=='d') {
        loglevel = 0xffffffff;
    } else if (c=='h') {
        ESP_LOGI(TAG, "Disable ULAHACK");
        fpga__clear_flags(FPGA_FLAG_ULAHACK);
    } else if (c=='H') {
        ESP_LOGI(TAG, "Enable ULAHACK");
        fpga__set_flags(FPGA_FLAG_ULAHACK);
    }
#endif
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
