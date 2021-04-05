#include "fpga.h"
#include "basickey.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom_hook.h"
#include "log.h"

#define TAG "REMOTETAP"

static int8_t hook128reset = -1;

void remotetap__prepareload()
{
    fpga__set_flags(FPGA_FLAG_RSTSPECT);
    fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_OFF | FPGA_FLAG_TRIG_FORCENMI_OFF);


    // Check if we are running in 128K mode. We need to switch to 48K ROM

    if (hook128reset<0) {
        hook128reset = rom_hook__add_pre_set(0, 0x00C7, 1); // ORG $00C7
    }

    const uint8_t loadbuf[] = {
        0xEF, // LOAD
        0x22, // "
        0x22, // "
        0x0d  // ENTER
    };
    basickey__inject(loadbuf, 4);

    vTaskDelay(fpga__get_reset_time() / portTICK_RATE_MS);
    fpga__clear_flags(FPGA_FLAG_RSTSPECT);
}

void remotetap__128resetcallback()
{
    ESP_LOGI(TAG, "128K reset successfuly to 48K mode");
    if (hook128reset>=0) {
        rom_hook__remove(hook128reset);
        hook128reset = -1;
    }
}

#if 0
esp_err_t remotetap__http_init();
{
    esp_err_t err = 0;
    do {
        err = httpd_query_key_value(querystr, "method", val, sizeof(val));
        if (err) {
            error = "Missing TAP method";
            break;
        }

        err = httpd_query_key_value(querystr, "type", val, sizeof(val));
        if (err) {
            error = "Missing TAP type";
            break;
        }
        // check tap, tzx, etc.

        req->content_len;

    } while (0);
}

#endif

