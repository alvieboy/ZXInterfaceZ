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
typedef int (*spectcmd_handler_t)(const uint8_t *cmdbuf, unsigned len);

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

static int spectcmd__do_load_resource(struct resource *r)
{
    return resource__sendtofifo(r);
}

#define NEED(x) do {  \
    if (len<x) { \
    return -1; \
    } \
    len-=x; \
} while (0);

static int spectcmd__set_filter(const uint8_t *cmdbuf, unsigned len)
{
    NEED(1);

    spectcmd__removedata();

    directory_resource__set_filter(&directoryresource, cmdbuf[0]);

    return 0;
}


static int spectcmd__load_resource(const uint8_t *cmdbuf, unsigned len)
{
    int ret = 0;
    struct resource *r;
    uint8_t error_resp = 0xff;

    NEED(1);

    spectcmd__removedata();

    r = resource__find(cmdbuf[0]);
    if (r!=NULL) {
        ESP_LOGI(TAG, "Found internal resource");
        return spectcmd__do_load_resource(r);
    } else {
        // Send null.
        ret = fpga__load_resource_fifo(&error_resp, sizeof(error_resp), RESOURCE_DEFAULT_TIMEOUT);
        if (ret<0)
            ESP_LOGI(TAG, "Resource %d not found", cmdbuf[0]);
    }
    return ret;
}

static int spectcmd__savesna(const uint8_t *cmdbuf,unsigned len)
{
    char filename[64];
    int ret = 0;

    NEED(1);

    uint8_t filelen = cmdbuf[0];

    NEED(filelen);

    spectcmd__removedata();

    opstatus__set_status( OPSTATUS_IN_PROGRESS, "");
    ESP_LOGI(TAG, "Saving snapshot file len %d", filelen);

    if (filelen && filelen<63) {
        memcpy(filename, &cmdbuf[1], filelen);
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

static int spectcmd__enterdir(const uint8_t *cmdbuf, unsigned len)
{
    int ret = 0;

    NEED(1);

    uint8_t dirlen = cmdbuf[0];

    NEED(dirlen);

    opstatus__set_status(OPSTATUS_IN_PROGRESS,"");

    // NULL-terminate string
    command_buffer[__cmdptr] = '\0';
    ESP_LOGI(TAG,"Chdir request to '%s'", &cmdbuf[1]);
    if (__chdir((const char*)&cmdbuf[1])!=0) {
        opstatus__set_status(OPSTATUS_ERROR,"Cannot chdir");
    } else {
        opstatus__set_status(OPSTATUS_SUCCESS,"");
    }

    spectcmd__removedata();

    return ret;
}

static int spectcmd__loadsna(const uint8_t *cmdbuf, unsigned len)
{
    char filename[32];
    int ret = 0;

    NEED(1);

    uint8_t filenamelen = cmdbuf[0];

    NEED(filenamelen);

    ESP_LOGI(TAG,"Loading snapshot from filesystem");

    spectcmd__removedata();

    opstatus__set_status(OPSTATUS_IN_PROGRESS,"");

    memcpy(filename, &cmdbuf[1], filenamelen);
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

static int spectcmd__playtape(const uint8_t *cmdbuf, unsigned len)
{
    char filename[32];
    int ret = 0;

    NEED(1);

    uint8_t filenamelen = cmdbuf[0];

    NEED(filenamelen);

    ESP_LOGI(TAG,"Playing tape");

    spectcmd__removedata();

    opstatus__set_status(OPSTATUS_IN_PROGRESS,"");

    memcpy(filename, &cmdbuf[1], filenamelen);
    filename[filenamelen] = '\0';

    tapeplayer__play(filename);

    return ret;
}

static int spectcmd__setvideomode(const uint8_t *cmdbuf, unsigned len)
{
    int ret = 0;

    NEED(1);

    uint8_t mode = cmdbuf[0];

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

static int spectcmd__setwifi(const uint8_t *cmdbuf, unsigned len)
{
    const uint8_t *buf = cmdbuf;
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

static int spectcmd__reset(const uint8_t *cmdbuf, unsigned len)
{
    return fpga__reset_spectrum();
}

static const spectcmd_handler_t spectcmd_handlers[] = {
    &spectcmd__load_resource, // 00 SPECTCMD_CMD_GETRESOURCE
    &spectcmd__setwifi,       // 01 SPECTCMD_CMD_SETWIFI
    NULL,                     // 02 SPECTCMD_CMD_STARTSCAN
    &spectcmd__savesna,       // 03 SPECTCMD_CMD_SAVESNA
    NULL,                     // 04 TBD
    NULL,                     // 05 TBD
    &spectcmd__loadsna,       // 06 SPECTCMD_CMD_LOADSNA
    &spectcmd__set_filter,    // 07 SPECTCMD_CMD_SETFILEFILTER
    &spectcmd__playtape,      // 08 SPECTCMD_CMD_PLAYTAPE
    NULL,                     // 09 SPECTCMD_CMD_RECORDTAPE
    NULL,                     // 0A SPECTCMD_CMD_STOPTAPE
    &spectcmd__enterdir,      // 0B SPECTCMD_CMD_ENTERDIR
    &spectcmd__setvideomode,  // 0C SPECTCMD_CMD_SETVIDEOMODE
    &spectcmd__reset,         // 0D SPECTCMD_CMD_RESET
    // FOPEN
    // FCLOSE
    // FREAD
    // FWRITE
    // READDIR
    // CWD
};

static int spectcmd__check()
{
    int ret = 0;
    uint8_t error_resp = 0xff;
    uint8_t index = command_buffer[0];

    ESP_LOGI(TAG,"Command in: %02x", index);

    spectcmd_handler_t handler = NULL;

    if (index<sizeof(spectcmd_handlers)/sizeof(spectcmd_handlers[0])) {
        handler = spectcmd_handlers[index];
    }

    if (handler) {
        ret = handler(&command_buffer[1], __cmdptr-1);
    } else {
        ESP_LOGI(TAG, "Invalid command 0x%02x", index);
        spectcmd__removedata();
        ret = fpga__load_resource_fifo(&error_resp, sizeof(error_resp), RESOURCE_DEFAULT_TIMEOUT);
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
