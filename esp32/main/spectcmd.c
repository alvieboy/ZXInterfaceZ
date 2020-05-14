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

#define COMMAND_BUFFER_MAX 128
static uint8_t command_buffer[COMMAND_BUFFER_MAX]; // Max 128 bytes.
static uint8_t cmdptr = 0;

void spectcmd__init()
{
    cmdptr = 0;
}

static void spectcmd__ackinterrupt()
{
    fpga__set_trigger(FPGA_FLAG_TRIG_INTACK);
}

static int spectcmd__loadresource(struct resource *r)
{
    return resource__sendtofifo(r);
}

static int spectcmd__set_filter()
{
    if (cmdptr<2) {
        return -1;
    }

    cmdptr = 0;

    directory_resource__set_filter(&directoryresource, command_buffer[1]);

    return 0;
}


static int spectcmd__load_resource()
{
    int ret = 0;
    struct resource *r;
    uint8_t error_resp = 0xff;

    if (cmdptr<2) {
        return ret;
    }

    cmdptr = 0;

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

static int spectcmd__savesna()
{
    char filename[64];
    int ret = 0;
    if (cmdptr<2) {
        return ret;
    }

    uint8_t filelen = command_buffer[1];
    int psize = 2+filelen;

    if (cmdptr < psize)
        return ret;

    cmdptr = 0;

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

static int spectcmd__enterdir()
{
    int ret = 0;
    if (cmdptr<2) {
        return ret;
    }

    uint8_t dirlen = command_buffer[1];
    int psize = 2+dirlen;

    if (cmdptr < psize)
        return ret;

    opstatus__set_status(OPSTATUS_IN_PROGRESS,"");

    // NULL-terminate string
    command_buffer[cmdptr] = '\0';
    ESP_LOGI(TAG,"Chdir request to '%s'", &command_buffer[2]);
    if (__chdir((const char*)&command_buffer[2])!=0) {
        opstatus__set_status(OPSTATUS_ERROR,"Cannot chdir");
    } else {
        opstatus__set_status(OPSTATUS_SUCCESS,"");
    }
    cmdptr = 0;
    return ret;
}

static int spectcmd__loadsna()
{
    char filename[32];
    int ret = 0;
    if (cmdptr<2) {
        return ret;
    }

    uint8_t filenamelen = command_buffer[1];
    int psize = 2+filenamelen;

    if (cmdptr < psize)
        return ret;

    ESP_LOGI(TAG,"Loading snapshot from filesystem");
    cmdptr = 0;

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


static int spectcmd__check()
{
    int ret = 0;
    uint8_t error_resp = 0xff;

 //   ESP_LOGI(TAG,"Command in: %02x", command_buffer[0]);
    switch (command_buffer[0]) {
    case SPECTCMD_CMD_GETRESOURCE:
        ret = spectcmd__load_resource();
        break;
    case SPECTCMD_CMD_SAVESNA:
        ret = spectcmd__savesna();
        break;
    case SPECTCMD_CMD_ENTERDIR:
        ret = spectcmd__enterdir();
        break;
    case SPECTCMD_CMD_SETFILEFILTER:
        ret = spectcmd__set_filter();
        break;
    case SPECTCMD_CMD_LOADSNA:
        ret = spectcmd__loadsna();
        break;
    default:
        ESP_LOGI(TAG, "Invalid command 0x%02x", command_buffer[0]);
        cmdptr=0;
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
        if (cmdptr<sizeof(command_buffer)) {
            command_buffer[cmdptr++] = r & 0xff;
            spectcmd__check();
        } else {
            ESP_LOGW(TAG, "Command buffer overflow!");
        }
    }

    // Clear IRQ.

}
