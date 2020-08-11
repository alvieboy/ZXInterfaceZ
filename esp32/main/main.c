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
#include "sdcard.h"
#include "tapeplayer.h"
#include "rom.h"
#include "usbh.h"
#include "usb_ll.h"
#include <esp32/rom/uart.h>
#include "keyboard.h"
#include "webserver.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "devmap.h"
#include "audio.h"
#include "console.h"
#include "version.h"

static int8_t videomode = 0;

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

void gpio__press_event(gpio_num_t gpio)
{
    if (gpio==PIN_NUM_SWITCH) {
        //fpga__clear_flags(FPGA_FLAG_ULAHACK);

        ESP_LOGI(TAG, "Requesting NMI!");
        fpga__set_trigger(FPGA_FLAG_TRIG_FORCENMI_ON);

        if (gpio==PIN_NUM_SWITCH) {
            uint16_t pc = fpga__get_spectrum_pc();
            ESP_LOGI(TAG, "Spectrum PC: 0x%04x",pc);
        }
    }
    if (gpio==PIN_NUM_IO0) {
    }
}

uint32_t loglevel;

void app_main()
{
    int lstatus = 0;
    int do_restart = 0;

    gpio__init();
    led__set(LED1, 1);
    led__set(LED2, 0);
    spi__init_bus();
    sdcard__init();
    nvs__init();
    wifi__init();


    if (fpga__init()<0) {
        ESP_LOGE(TAG, "Cannot proceed without FPGA!");
        while (1) {
            led__set(LED1, 1);
            vTaskDelay(100 / portTICK_RATE_MS);
            led__set(LED1, 0);
            vTaskDelay(100 / portTICK_RATE_MS);

        }
    }

    spectint__init();

    resource__init();

    resource__register( 0x00, &versionresource);
    resource__register( 0x02, &statusresource.r);
    resource__register( 0x03, &directoryresource.r);
    resource__register( 0x04, &wificonfigresource.r);
    resource__register( 0x05, &aplistresource.r);

    videomodeconfigresource.valptr = &videomode;

    resource__register( 0x06, &videomodeconfigresource.r);
    resource__register( 0x10, &opstatusresource.r);

    if (rom__load_from_flash()!=0) {
        ESP_LOGW(TAG,"Cannot load ROM from flash, continuing with no ROM");
    }

    config__init();
    spectcmd__init();
    videostreamer__init();
    netcmd__init();
    tapeplayer__init();
    devmap__init();
    usbh__init();
    keyboard__init();
    audio__init();
    webserver__init();

    ESP_LOGI(TAG, "InterfaceZ version: %s %s", version, gitversion);
    ESP_LOGI(TAG, "  built %s", builddate);

    // Start capture
    //start_capture();
    unsigned iter = 0;
    while(1) {
        uint8_t c;
        led__set(LED1, !!(lstatus&0x4));
        lstatus++;
        /*int sw = gpio_get_level(PIN_NUM_SWITCH);
        if (sw==0 && lastsw==1) {
            ESP_LOGI(TAG, "Start capture");
            start_capture2();
        }
        lastsw = sw;
        */
        if (restart_requested) {
            do_restart = 20;
            restart_requested = 0;
        }

        vTaskDelay(100 / portTICK_RATE_MS);

        if (do_restart==1) {
            esp_restart();
        } else if (do_restart>0) {
            printf("Restart tick %d\n", do_restart);
            do_restart--;
        }


        fpga__get_status();
        iter++;
        if (iter==50) {
            // 5-second mark.
            //ESP_LOGI(TAG,"Interrupts: %d", videostreamer__getinterrupts());
            iter=0;
        }
        STATUS s;
        do {
            s = uart_rx_one_char(&c);
            if (s==OK) {
                console__char(c);
            }
        } while (s==OK);

    }
}
