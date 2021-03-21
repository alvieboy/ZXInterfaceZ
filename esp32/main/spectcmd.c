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
#include "vga.h"
#include "spectrum_kbd.h"
#include "wsys.h"
#include "fasttap.h"
#include "esp32/rom/crc.h"
#include "rom.h"
#include "log.h"
#include "esxdos.h"
#include "wsys.h"
#include "tapeplayer.h"
#include "save.h"
#include "memlayout.h"
#include "spectctrl.h"
#include "tape.h"
#include "debugger.h"
#include "memdata.h"
#include "interfacez_tasks.h"

#define COMMAND_BUFFER_MAX 256+2

static uint8_t command_buffer[COMMAND_BUFFER_MAX]; // Max 256+2 bytes.
static uint16_t __cmdptr = 0;
typedef int (*spectcmd_handler_t)(const uint8_t *cmdbuf, unsigned len);

static xQueueHandle data_queue = NULL;

static void spectcmd__task(void *pvParam);


static void spectcmd__removedata()
{
    __cmdptr = 0;
}

void spectcmd__init()
{
    // Create handler task

    spectcmd__removedata();

    data_queue  = xQueueCreate(64, sizeof(uint8_t));
    xTaskCreate(spectcmd__task, "spectcmd_task", SPECTCMD_TASK_STACK_SIZE, NULL, SPECTCMD_TASK_PRIORITY, NULL);
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

    //directory_resource__set_filter(&directoryresource, cmdbuf[0]);

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

    ret = sna__load_snapshot_extram(filename);

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

static int spectcmd__playtape_fast(const uint8_t *cmdbuf, unsigned len)
{
    char filename[32];

    NEED(1);

    uint8_t filenamelen = cmdbuf[0];

    NEED(filenamelen);

    ESP_LOGI(TAG,"Playing tape (fast)");

    spectcmd__removedata();

    memcpy(filename, &cmdbuf[1], filenamelen);
    filename[filenamelen] = '\0';

    return fasttap__prepare(filename);
}

static int spectcmd__setvideomode(const uint8_t *cmdbuf, unsigned len)
{
    NEED(1);

    uint8_t mode = cmdbuf[0];

    spectcmd__removedata();

    return vga__setmode(mode);
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
    spectctrl__reset();
    spectcmd__removedata();
    return 0;
}

static int spectcmd__kbddata(const uint8_t *cmdbuf, unsigned len)
{
    uint16_t key;

    NEED(2);

    key = *cmdbuf++;
    key += (uint16_t)(*cmdbuf)<<8;

    ESP_LOGD(TAG, "KBD: %04x", key);

    unsigned char c = spectrum_kbd__to_ascii(key);
    //if (c >= ' ') {
        //ESP_LOGI(TAG," > Key '%c'", c);
    //}
    wsys__keyboard_event(key, c);

    spectcmd__removedata();
    return 0;
}

static int spectcmd__nmiready(const uint8_t *cmdbuf, unsigned len)
{
    spectcmd__removedata();
    ESP_LOGI(TAG, "NMI ready");
    wsys__nmiready();
    return 0;
}

static int spectcmd__leavenmi(const uint8_t *cmdbuf, unsigned len)
{
    spectcmd__removedata();
    ESP_LOGI(TAG, "NMI finished");
    wsys__nmileave();
    return 0;
}

static int spectcmd__detect(const uint8_t *cmdbuf, unsigned len)
{
    uint8_t model;
    uint8_t flags;

    NEED(2);

    model = *cmdbuf++;
    flags = *cmdbuf++;

    spectrum_model_detected(model, flags);

    spectcmd__removedata();
    return 0;
}

static int spectcmd__fast_load(const uint8_t *cmdbuf, unsigned len)
{
    spectcmd__removedata();

    // Activate ROMCS
    fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_ON);

    return 0;
}

static int spectcmd__fast_load_data(const uint8_t *cmdbuf, unsigned len)
{
    NEED(3);
    uint16_t blocklen = cmdbuf[1];
    blocklen +=( (unsigned)cmdbuf[2])<<8;
    ESP_LOGI(TAG, "TAP: requested fast load %02x size %d", cmdbuf[0], blocklen);
    spectcmd__removedata();
    fasttap__next(cmdbuf[0], blocklen);
    return 0;
}

#define CRCPOLY_LE 0xedb88320

static uint32_t rom__crc32_le(uint32_t crc, unsigned char const *p, size_t len)
{
    int i;
    while (len--) {
        crc ^= *p++;
        for (i = 0; i < 8; i++)
            crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
    }
    return crc;
}


static int spectcmd__romcrc(const uint8_t *cmdbuf, unsigned len)
{
    NEED(257);
    uint8_t romnum = *cmdbuf++;

    uint32_t crc = 0xFFFFFFFF;

    crc = rom__crc32_le(crc, cmdbuf, 256);

    ESP_LOGI(TAG, "Dump for ROM%d", romnum);
    do {
        unsigned offset=0;
        char title[16];
        while (offset<256) {
            sprintf(title, "%03d: ", offset);
            BUFFER_LOGI(TAG, title, &cmdbuf[offset], 16);
            offset+=16;
        }
    } while (0);
    /*
    ESP_LOGI(TAG,"First rom %d 8 bytes (64-apart): %02x %02x %02x %02x %02x %02x %02x %02x",
             romnum,
             cmdbuf[0],
             cmdbuf[1],
             cmdbuf[2],
             cmdbuf[3],
             cmdbuf[4],
             cmdbuf[5],
             cmdbuf[6],
             cmdbuf[7]);
      */
    //crc = crc32_le(crc, cmdbuf, 256);
    crc = crc ^ 0xFFFFFFFF;

    const rom_model_t *model = rom__set_rom_crc(romnum, crc);

    ESP_LOGI(TAG, "ROM %d CRC: %08x : %s", romnum, crc, model ? model->name: "unknown");


    spectcmd__removedata();
    return 0;
}


static int spectcmd__esxdos_diskinfo(const uint8_t *cmdbuf, unsigned len)
{
    NEED(1);
    int r = esxdos__diskinfo(cmdbuf[0]);
    spectcmd__removedata();
    return r;
}

static int spectcmd__esxdos_driveinfo(const uint8_t *cmdbuf, unsigned len)
{
    NEED(1);
    int r = esxdos__driveinfo(cmdbuf[0]);
    spectcmd__removedata();
    return r;
}

static int spectcmd__esxdos_close(const uint8_t *cmdbuf, unsigned len)
{
    NEED(1);
    int r = esxdos__close(cmdbuf[0]);
    spectcmd__removedata();
    return r;
}

static int spectcmd__esxdos_open(const uint8_t *cmdbuf, unsigned len)
{
    char filename[255];
    NEED(2);
    uint8_t mode        = cmdbuf[0];
    uint8_t filenamelen = cmdbuf[1];

    NEED(filenamelen);
    memcpy(filename, &cmdbuf[2], filenamelen);
    filename[filenamelen] = '\0';

    spectcmd__removedata();

    int r = esxdos__open(filename, mode);

    return r;
}

static int spectcmd__esxdos_read(const uint8_t *cmdbuf, unsigned len)
{
    NEED(3);
    uint8_t fh = cmdbuf[0];
    uint16_t readlen = (uint16_t)cmdbuf[1] + (((uint16_t)cmdbuf[2])<<8);
    spectcmd__removedata();
    return esxdos__read(fh, readlen);
}

static int spectcmd__loadtrap(const uint8_t *cmdbuf, unsigned len)
{
    uint8_t status = SPECTCMD_CMD_LOADTRAP;
    spectcmd__removedata();


    wsys__reset(WSYS_MODE_LOAD);
    fpga__write_miscctrl(0x01); // Regular NMI handling (i.e., ends with RETN). This notifies the ROM firmware

    // Ack with 0xFF if we need to return to spectrum immediatly.
    if (fasttap__is_playing() || tapeplayer__isplaying())
        status = 0xff;

    ESP_LOGI(TAG, "Loading status %d", status);
    fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);

    return 0;
}

static int spectcmd__savetrap(const uint8_t *cmdbuf, unsigned len)
{
    uint8_t status;
    NEED(3);
    uint16_t size = (((uint16_t)cmdbuf[0])<<8) + cmdbuf[1];
    uint8_t type = cmdbuf[2];

    if (type==0) {
        ESP_LOGI(TAG,"Save %d bytes, wait header\n", size);
        NEED(size);
    }

    ESP_LOGI(TAG,"Request save %d bytes, type %d\n", size, type);

    spectcmd__removedata();

    if (size==18) {
        save__set_data_from_header(&cmdbuf[3], size);
    }

    switch (tape__get_tape_mode()) {

    case TAPE_NO_TAPE:
        wsys__reset(WSYS_MODE_SAVE);
        fpga__write_miscctrl(0x01); // Non-regular NMI handling (i.e., ends with RET). This notifies the ROM firmware
        status = SPECTCMD_CMD_SAVETRAP;
        fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);
        break;
    case TAPE_PHYSICAL_SAVE:
        // Saving to physical tape.
        status = 0xFF; // Don't enter menu
        fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);
        status = 0xFF; // Don't copy data
        fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);
        break;
    case TAPE_TAP_SAVE: /* Fall-through */
    case TAPE_TZX_SAVE:
        status = 0xFF; // Don't enter menu
        fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);
        status = 0x00; // copy data
        fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);
        break;
    default:
        // Playing ?
        break;
    }
    return 0;
}

static int spectcmd__savedata(const uint8_t *cmdbuf, unsigned len)
{
    NEED(2);
    uint16_t size = (((uint16_t)cmdbuf[0])<<8) + cmdbuf[1];

    //ESP_LOGI(TAG,"Request save data %d bytes", size);
    spectcmd__removedata();

    save__append_from_extram(MEMLAYOUT_TAPE_WORKAREA, size);

    return 0;
}

static int spectcmd__memdata(const uint8_t *cmdbuf, unsigned len)
{
    NEED(1);
    uint8_t size = cmdbuf[0];

    NEED(size);

    ESP_LOGI(TAG,"Memory data %d bytes ready", size);

    spectcmd__removedata();

    memdata__store(&cmdbuf[1], size);


    wsys__memoryreadcomplete(size);

    return 0;
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
    &spectcmd__kbddata,       // 0E SPECTCMD_CMD_KBDDATA
    &spectcmd__nmiready,      // 0F SPECTCMD_CMD_NMIREADY
    &spectcmd__leavenmi,      // 10 SPECTCMD_CMD_LEAVENMI
    &spectcmd__detect,        // 11 SPECTCMD_CMD_SPECTRUM_DETECT
    &spectcmd__fast_load,     // 12 SPECTCMD_CMD_FASTLOAD
    &spectcmd__fast_load_data,// 13 SPECTCMD_CMD_FASTLOAD_DATA
    &spectcmd__playtape_fast, // 14 SPECTCMD_CMD_PLAYTAPE_FAST
    &spectcmd__romcrc,        // 15 SPECTCMD_CMD_ROMCRC
    &spectcmd__loadtrap,      // 16 SPECTCMD_CMD_LOADTRAP
    &spectcmd__savetrap,      // 17 SPECTCMD_CMD_SAVETRAP
    &spectcmd__savedata,      // 18 SPECTCMD_CMD_SAVEDATA,
    &spectcmd__memdata,       // 19 SPECTCMD_CMD_MEMDATA,
    NULL,                     // 1A
    NULL,                     // 1B
    NULL,                     // 1C
    NULL,                     // 1D
    NULL,                     // 1E
    NULL,                     // 1F
    &spectcmd__esxdos_diskinfo, // 20
    &spectcmd__esxdos_driveinfo, // 21
    &spectcmd__esxdos_open, // 22
    &spectcmd__esxdos_read, // 23
    &spectcmd__esxdos_close, // 24
};


static int spectcmd__check()
{
    int ret = 0;
    uint8_t error_resp = 0xff;
    uint8_t index = command_buffer[0];

    //ESP_LOGI(TAG,"Command in: %02x ptr %d", index, __cmdptr);

    spectcmd_handler_t handler = NULL;

    if (index<sizeof(spectcmd_handlers)/sizeof(spectcmd_handlers[0])) {
        handler = spectcmd_handlers[index];
    }

    if (handler) {
        //ESP_LOGI(TAG, "Dispatching command");
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
    union u32 data;
    uint8_t *dptr;

    while (1) {
        dptr = &data.w8[0];
        r = fpga__read_command_fifo(&data.w32);

        if (r<0) {
            break; // No more data
        }
        while (r--) {
            xQueueSend(data_queue, dptr, 100);
            dptr++;
        }
    }

}

static void spectcmd__task(void *pvParam)
{
    uint8_t cmd;

    while (1) {
        if (xQueueReceive(data_queue, &cmd, 1000)==pdTRUE) {

            if (__cmdptr<sizeof(command_buffer)) {
#if 0
                ESP_LOGI(TAG, "Buf %02x %02x %02x %02x",
                         cmdbuf[0],
                         cmdbuf[1],
                         cmdbuf[2],
                         cmdbuf[3]);
#endif
#if 0
                ESP_LOGI(TAG, "Queueing %02x (%d)", cmd, __cmdptr);
#endif
                command_buffer[__cmdptr++] = cmd;
                spectcmd__check();
            } else {
                ESP_LOGW(TAG, "Command buffer overflow!");
            }
        }
    }

}
