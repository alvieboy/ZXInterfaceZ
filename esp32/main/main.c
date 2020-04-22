#include "spi.h"
#include "fpga.h"
#include "flash_pgm.h"
#include "dump.h"
#include "command.h"
#include "ota.h"
#include "fpga_ota.h"
#include "sna.h"
#include "res.h"
#include "strtoint.h"
#include "netcomms.h"
#include "spectcmd.h"
#include "wifi.h"
#include "led.h"
#include "nvs.h"
#include "gpio.h"
#include "interfacez_resources.h"
#include "spectint.h"
#include "videostreamer.h"
#include "netcmd.h"


volatile int restart_requested = 0;

#if 0
static void start_capture2()
{
    // Stop, clear
    fpga__set_clear_flags( FPGA_FLAG_CAPCLR, FPGA_FLAG_CAPRUN);
    fpga__set_flags( FPGA_FLAG_CAPRUN);
}

static void start_capture()
{
    // Reset spectrum, stop capture
    fpga__set_clear_flags( FPGA_FLAG_RSTSPECT, FPGA_FLAG_CAPRUN );
    // Reset fifo, clear capture
    fpga__set_flags( FPGA_FLAG_CAPCLR | FPGA_FLAG_RSTFIFO);

    // Remove resets, start capture
    fpga__set_clear_flags( FPGA_FLAG_CAPRUN | FPGA_FLAG_ENABLE_INTERRUPT , FPGA_FLAG_RSTFIFO| FPGA_FLAG_RSTSPECT| FPGA_FLAG_CAPCLR );

}
#endif


void request_restart()
{
    restart_requested = 1;
}

void app_main()
{
    int lstatus = 0;
   // int lastsw = 1;
    int do_restart = 0;

    gpio__init();
    led__set(LED1, 1);
    led__set(LED2, 0);
    spi__init_bus();

//    sdcard__init();
    nvs__init();
    wifi__init();


    fpga__init();
    resource__init();

    resource__register( 0x00, &versionresource);
    resource__register( 0x02, &statusresource.r);
    resource__register( 0x04, &wificonfigresource.r);
    resource__register( 0x05, &aplistresource.r);

    spectcmd__init();
    flash_pgm__init();

    videostreamer__init();
    netcmd__init();
    spectint__init();

    // Start capture
    //start_capture();

    while(1) {
        led__set(LED1, lstatus);
        lstatus = !lstatus;
        /*int sw = gpio_get_level(PIN_NUM_SWITCH);
        if (sw==0 && lastsw==1) {
            ESP_LOGI(TAG, "Start capture");
            start_capture2();
        }
        lastsw = sw;
        */
        if (restart_requested)
            do_restart = 1;

        vTaskDelay(1000 / portTICK_RATE_MS);

       // ESP_LOGI(TAG,"Interrupts: %d\n", (int)interrupt_count);
       // ESP_LOGI(TAG, "CMD IN %d", gpio_get_level(PIN_NUM_CMD_INTERRUPT));

        if (do_restart)
            esp_restart();

    }
}
