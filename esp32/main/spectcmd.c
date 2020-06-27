#include "fpga.h"
#include "spectcmd.h"
#include "version.h"
#include "resource.h"
#include "esp_log.h"
#include "defs.h"
#include "opstatus.h"
#include "sna.h"
#include <unistd.h>
#include "fileaccess.h"
#include "interfacez_resources.h"
#include "sna.h"
#include "tapeplayer.h"
#include "wifi.h"

#define COMMAND_BUFFER_MAX 128
static uint8_t command_buffer[COMMAND_BUFFER_MAX]; // Max 128 bytes.
static uint8_t __cmdptr = 0;

static void spectcmd__removedata()
{
    __cmdptr = 0;
}

void spectcmd__init()
{
    spectcmd__removedata();
}

static void spectcmd__ackinterrupt()
{
    fpga__set_trigger(FPGA_FLAG_TRIG_INTACK);
}

static int spectcmd__loadresource(struct resource *r)
{
    return resource__sendtofifo(r);
}

#define NEED(x) do {  \
    if (len<x) { \
    return -1; \
    } \
    len-=x; \
} while (0);

static int spectcmd__set_filter(unsigned len)
{
    NEED(1);

    spectcmd__removedata();

    directory_resource__set_filter(&directoryresource, command_buffer[1]);

    return 0;
}


static int spectcmd__load_resource(unsigned len)
{
    int ret = 0;
    struct resource *r;
    uint8_t error_resp = 0xff;

    NEED(1);

    spectcmd__removedata();

    r = resource__find(command_buffer[1]);
    if (r!=NULL) {
        //    ESP_LOGI(TAG, "Found internal resource");
        return spectcmd__loadresource(r);
    } else {
        // Send null.
        ret = fpga__load_resource_fifo(&error_resp, sizeof(error_resp), RESOURCE_DEFAULT_TIMEOUT);
        if (ret<0)
            ESP_LOGI(TAG, "Resource %d not found", command_buffer[1]);
    }
    return ret;
}

static int spectcmd__savesna(unsigned len)
{
    char filename[64];
    int ret = 0;

    NEED(1);

    uint8_t filelen = command_buffer[1];

    NEED(filelen);

    spectcmd__removedata();

    opstatus__set_status( OPSTATUS_IN_PROGRESS, "");
    ESP_LOGI(TAG, "Saving snapshot file len %d", filelen);

    if (filelen && filelen<63) {
        memcpy(filename, &command_buffer[2], filelen);
        strcpy(&filename[filelen], ".sna");
        ret = sna__save_from_extram(filename);
    } else {
        ret = sna__save_from_extram("unnamed.sna");
    }


    if (ret==0) {
        ESP_LOGI(TAG, "Snapshot saved");
        opstatus__set_status( OPSTATUS_OK, "Snapshot saved");
    } else {
        ESP_LOGE(TAG, "Error saving snapshot");

        opstatus__set_error(sna__get_error_string());
    }

    return ret;
}

static int spectcmd__enterdir(unsigned len)
{
    int ret = 0;

    NEED(1);

    uint8_t dirlen = command_buffer[1];

    NEED(dirlen);

    opstatus__set_status(OPSTATUS_IN_PROGRESS,"");

    // NULL-terminate string
    command_buffer[__cmdptr] = '\0';
    ESP_LOGI(TAG,"Chdir request to '%s'", &command_buffer[2]);
    if (__chdir((const char*)&command_buffer[2])!=0) {
        opstatus__set_status(OPSTATUS_ERROR,"Cannot chdir");
    } else {
        opstatus__set_status(OPSTATUS_SUCCESS,"");
    }

    spectcmd__removedata();

    return ret;
}

static int spectcmd__loadsna(unsigned len)
{
    char filename[32];
    int ret = 0;

    NEED(1);

    uint8_t filenamelen = command_buffer[1];

    NEED(filenamelen);

    ESP_LOGI(TAG,"Loading snapshot from filesystem");

    spectcmd__removedata();

    opstatus__set_status(OPSTATUS_IN_PROGRESS,"");

    memcpy(filename, &command_buffer[2], filenamelen);
    filename[filenamelen] = '\0';

    ret = sna__load_sna_extram(filename);

    if (ret==0) {
        ESP_LOGI(TAG, "Snapshot loaded into RAM");
        opstatus__set_status(OPSTATUS_SUCCESS,"");
    } else {
        ESP_LOGI(TAG, "Snapshot load error");
        opstatus__set_status(OPSTATUS_ERROR,"Cannot load SNA");
    }
    return ret;
}

static int spectcmd__playtape(unsigned len)
{
    char filename[32];
    int ret = 0;

    NEED(1);

    uint8_t filenamelen = command_buffer[1];

    NEED(filenamelen);

    ESP_LOGI(TAG,"Playing tape");

    spectcmd__removedata();

    opstatus__set_status(OPSTATUS_IN_PROGRESS,"");

    memcpy(filename, &command_buffer[2], filenamelen);
    filename[filenamelen] = '\0';

    tapeplayer__play(filename);

    return ret;
}

static int spectcmd__setvideomode(unsigned len)
{
    int ret = 0;

    NEED(1);

    uint8_t mode = command_buffer[1];

    if (mode>3)
        mode = 0;

    spectcmd__removedata();

    switch (mode) {
    case 0:
        fpga__clear_flags(FPGA_FLAG_VIDMODE0 | FPGA_FLAG_VIDMODE1);
        break;
    case 1:
        fpga__set_clear_flags(FPGA_FLAG_VIDMODE0, FPGA_FLAG_VIDMODE1);
        break;
    case 2:
        fpga__set_clear_flags(FPGA_FLAG_VIDMODE1, FPGA_FLAG_VIDMODE0);
        break;
    case 3:
        fpga__set_flags(FPGA_FLAG_VIDMODE0 | FPGA_FLAG_VIDMODE1);
        break;
    }
    return ret;
}

static int spectcmd__setwifi(unsigned len)
{
    const uint8_t *buf = &command_buffer[1];
    const char *ssidstart;
    const char *pwdstart;
    char ssid[33];
    char pwd[33];
    uint8_t channel;

    NEED(1);

    uint8_t mode = *buf++;

    uint8_t ssidlen = *buf++;
    NEED(ssidlen);
    ssidstart = (const char*)buf;
    buf += ssidlen;
    uint8_t pwdlen = *buf++;
    NEED(pwdlen);
    pwdstart = (const char*)buf;
    buf += pwdlen;

    if ((ssidlen>32) || (pwdlen>32)) {
        spectcmd__removedata();
        return -1;
    }

    if (mode==0x00) { // AP
        NEED(1);
        channel = *buf++;
        strncpy(ssid, ssidstart, ssidlen);
        strncpy(pwd, pwdstart, pwdlen);

        return wifi__config_ap(ssid, pwd, channel);
    } else {
        // STA
        strncpy(ssid, ssidstart, ssidlen);
        strncpy(pwd, pwdstart, pwdlen);
        return wifi__config_sta(ssid, pwd);
    }

    spectcmd__removedata();
    return 0;
}

static int spectcmd__check()
{
    int ret = 0;
    uint8_t error_resp = 0xff;

    ESP_LOGI(TAG,"Command in: %02x", command_buffer[0]);
    switch (command_buffer[0]) {
    case SPECTCMD_CMD_GETRESOURCE:
        ret = spectcmd__load_resource(__cmdptr-1);
        break;
    case SPECTCMD_CMD_SAVESNA:
        ret = spectcmd__savesna(__cmdptr-1);
        break;
    case SPECTCMD_CMD_ENTERDIR:
        ret = spectcmd__enterdir(__cmdptr-1);
        break;
    case SPECTCMD_CMD_SETFILEFILTER:
        ret = spectcmd__set_filter(__cmdptr-1);
        break;
    case SPECTCMD_CMD_LOADSNA:
        ret = spectcmd__loadsna(__cmdptr-1);
        break;
    case SPECTCMD_CMD_PLAYTAPE:
        ret = spectcmd__playtape(__cmdptr-1);
        break;
    case SPECTCMD_CMD_SETVIDEOMODE:
        ret = spectcmd__setvideomode(__cmdptr-1);
        break;
    case SPECTCMD_CMD_SETWIFI:
        ret = spectcmd__setwifi(__cmdptr-1);
        break;
    default:
        ESP_LOGI(TAG, "Invalid command 0x%02x", command_buffer[0]);
        spectcmd__removedata();
        ret = fpga__load_resource_fifo(&error_resp, sizeof(error_resp), RESOURCE_DEFAULT_TIMEOUT);
        break;
    }

    return ret;
}


void spectcmd__request()
{
    int r;
    while (1) {
        r = fpga__read_command_fifo();
        spectcmd__ackinterrupt();

        if (r<0) {
            break; // No more data
        }
        if (__cmdptr<sizeof(command_buffer)) {
            command_buffer[__cmdptr++] = r & 0xff;
            spectcmd__check();
        } else {
            ESP_LOGW(TAG, "Command buffer overflow!");
        }
    }

    // Clear IRQ.

}
