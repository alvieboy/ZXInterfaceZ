#include "fpga.h"
#include "spectcmd.h"
#include "version.h"
#include "resource.h"
#include "esp_log.h"
#include "defs.h"

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

static int spectcmd__check()
{
    struct resource *r;
    int ret = 0;
    uint8_t error_resp = 0xff;

    ESP_LOGI(TAG,"Command in: %02x\n", command_buffer[0]);
    switch (command_buffer[0]) {
    case SPECTCMD_CMD_GETRESOURCE:
        cmdptr = 0;
        r = resource__find(command_buffer[0]);
        if (r!=NULL) {
            return spectcmd__loadresource(r);
        } else {
            // Send null.
            ret = fpga__load_resource_fifo(&error_resp, sizeof(error_resp), RESOURCE_DEFAULT_TIMEOUT);
        }
        break;
    default:
        ret = fpga__load_resource_fifo(&error_resp, sizeof(error_resp), RESOURCE_DEFAULT_TIMEOUT);
    }

    return ret;
}

void spectcmd__request()
{
    int r;
    while (cmdptr<sizeof(command_buffer)) {
        r = fpga__read_command_fifo();
        if (r<0) {
            break; // No more data
        }
        command_buffer[cmdptr++] = r & 0xff;
        spectcmd__check();
    }

    // Clear IRQ.

    spectcmd__ackinterrupt();
}
