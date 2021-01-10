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
#include "onchip_nvs.h"
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
#include "buttons.h"
#include "vga.h"
#include "memlayout.h"
#include "wsys.h"
#include "memlayout.h"
#include "adc.h"
#include "board.h"
#include "bit.h"
#include "hdlc_decoder.h"
#include "scope.h"
#include "model.h"
#include "divcompat.h"
#include "rom_hook.h"

static int8_t videomode = 0;

uint32_t loglevel;

volatile int restart_requested = 0;
static volatile uint8_t spectrum_model = 0xff;
static volatile uint8_t spectrum_flags = 0x00;

static void detect_spectrum(void);
void request_restart(void)
{
    restart_requested = 1;
}

static void io0_long_press()
{
    char romname[128];

    if (nvs__fetch_str("io0rom", romname, sizeof(romname), "/spiffs/DiagROMv54.rom")<0) {
        ESP_LOGE(TAG, "Cannot fetch rom for IO0");
        return ;
    }
    ESP_LOGI(TAG, "Loading ROM %s", romname);

    int f = rom__load_custom_from_file(romname, MEMLAYOUT_ROM2_BASEADDRESS);
    if (f<0) {
        ESP_LOGE(TAG, "Cannot load ROM %s", romname);
        return;
    }

    ESP_LOGI(TAG, "Resetting to custom ROM");
    if (fpga__reset_to_custom_rom(ROM_2, false)<0) {
        ESP_LOGE(TAG, "Cannot reset");
    }
}

static void load_esxdos()
{
    const char *romname = "/spiffs/esxdos2.rom";
    int f = rom__load_custom_from_file(romname, MEMLAYOUT_ROM1_BASEADDRESS);
    if (f<0) {
        ESP_LOGE(TAG, "Cannot load ROM %s", romname);
        return;
    }

    divcompat__enable(0); // 48K only for now

    ESP_LOGI(TAG, "Resetting to custom ROM");
    if (fpga__reset_to_custom_rom(ROM_1, false)<0) {
        ESP_LOGE(TAG, "Cannot reset");
    }

}

static void event_button_switch(button_event_type_e type)
{
    if (type==BUTTON_RELEASED) { // || type==BUTTON_LONG_RELEASED) {
        ESP_LOGI(TAG, "Requesting NMI!");
        wsys__reset(WSYS_MODE_NMI);
        fpga__write_miscctrl(0x00); // Regular NMI handling (i.e., ends with RETN). This notifies the ROM firmware
        fpga__set_trigger(FPGA_FLAG_TRIG_FORCENMI_ON);
    } else if (type==BUTTON_LONG_PRESSED) {
        ESP_LOGI(TAG, "Loading ESXDOS");
        load_esxdos();
    }
}

static void event_button_io0(button_event_type_e type)
{
    if (type==BUTTON_RELEASED || type==BUTTON_LONG_RELEASED) {
        ESP_LOGI(TAG, "IO0 released");
    }
    if (type==BUTTON_PRESSED) {
        ESP_LOGI(TAG, "IO0 pressed");
        //detect_spectrum();
    }
    if (type==BUTTON_LONG_PRESSED) {
        ESP_LOGI(TAG, "IO0 long pressed");
        io0_long_press();
    }
}

static void process_buttons()
{
    button_event_t event;
    if (buttons__getevent(&event, false)==0) {
        switch (event.button) {
        case BUTTON_SWITCH:
            event_button_switch(event.type);
            break;
        case BUTTON_IO0:
            event_button_io0(event.type);
        }
    }
}

void spectrum_model_detected(uint8_t model, uint8_t flags)
{
    spectrum_model = model;
    spectrum_flags = flags;
}

const char *spectrum_model_str(uint8_t model)
{
    const char *modelstr = "Unknown";
    switch (model) {
    case 0xfe:
        modelstr = "Unknown (detection error)";
        break;
    case 0x00:
        modelstr = "16/48K";
        break;
    case 0x01:
        if (board__is9Vsupply()) {
            modelstr = "+2(128K)";
        } else if (board__isplus2plus3supply()) {
            modelstr = "+2A,+3";
        } else {
            modelstr = "+2(128K),+2A,+3";
        }
        break;
    }
    return modelstr;
}

const char *get_spectrum_model(void)
{
    return spectrum_model_str(spectrum_model);
}

static uint32_t shift_lfsr(uint32_t data, const uint32_t xorpat)
{
    uint32_t shifted = data>>1;
    if (data&1) {
        shifted ^= xorpat;
    }
    return shifted;
}

static bool extram_test(bool simple_test)
{
    // 16MB. with 256-byte chunks, it gives 65536 iterations. 2080 bits for each row.
    // Takes about 30 seconds to test all RAM at 10Mbit/s.
#ifdef __linux__
    const unsigned topram = 0x00080000;
#else
    const unsigned topram = 0x00800000;
#endif


    if (simple_test)
    {
        union {
            uint32_t u32;
            uint8_t u8[4];
        } pat, readback;

        unsigned address = 0;
        pat.u32 = 0xDEADBEEF;
        const uint32_t xorpat = (1<<31) | (1<<21) | (1<<1) | (1<<0);

        ESP_LOGI(TAG, "Testing external RAM (simple)");


        while (address < topram) {
            //ESP_LOGI(TAG, "Write %08x value=%08x", address, pat.u32);
            fpga__write_extram_block(address, &pat.u8[0], sizeof(pat.u8));
            pat.u32 = shift_lfsr(pat.u32, xorpat);
            address += 0x1000;
        }

        // Read back
        pat.u32 = 0xDEADBEEF;
        address = 0;
        while (address < topram) {
            readback.u32 = 0;
            fpga__read_extram_block(address, &readback.u8[0], sizeof(readback.u8));
            if (readback.u32 != pat.u32) {
                ESP_LOGE(TAG, "Read error at address %08x: expected %08x, got %08x",
                         address,
                         pat.u32,
                         readback.u32);
                return false;
            }
            pat.u32= shift_lfsr(pat.u32, xorpat);
            address += 0x1000;
        }
        ESP_LOGI(TAG, "External RAM test (simple) passed");
        return true;
    }
    return false;
}

#define SPECTRUM_FLAGS_AY (1<<0)

static void detect_spectrum()
{
    spectrum_model = 0xfe;
    int timeout = 100;

    fpga__reset_to_custom_rom(ROM_0, false);
    // We need to wait for 1000ms,
    do {
        if (spectrum_model !=0xfe)
            break;
        vTaskDelay(100 / portTICK_RATE_MS);
    } while (--timeout);

    fpga__reset_spectrum();


    ESP_LOGI(TAG," AY-3-8912: %s", spectrum_flags& SPECTRUM_FLAGS_AY ? "PRESENT": "absent");

    switch(spectrum_model) {
    case 0xfe:
        break;
    case 0x00:
        model__set(MODEL_48K); //"16/48K";
        break;
    case 0x01:
        if (board__hasVoltageSensor()) {

            if (board__is9Vsupply()) {
                model__set(MODEL_128K); 
            } else if (board__isplus2plus3supply()) {
                model__set(MODEL_2APLUS);
            } else {
                // Unknown model
            }
        } else {
            // Must be 128K
            model__set(MODEL_128K); 
        }
        break;
    }

    ESP_LOGI(TAG,"Spectrum model: %s (0x%02x)", spectrum_model_str(spectrum_model), spectrum_model);


    if (nvs__u8("ay",1)) {
        uint32_t bits = CONFIG1_AY_ENABLE;
        // Enable reads from internal AY only and only if there is no
        // external (inside spectrum) AY chip.
        if (!(spectrum_flags & SPECTRUM_FLAGS_AY)) {
            bits |= CONFIG1_AY_READ_ENABLE;
            ESP_LOGI(TAG," AY-3-8912: Enabling AY reads");
        }
        fpga__set_config1_bits(bits);
    }
}

static hdlc_decoder_t console_hdlc_decoder;
static uint8_t console_hdlc_buf[64];

static void console_char_data(void *user, uint8_t val)
{
    console__char(val);
}

static void console_hdlc_data(void *user, const uint8_t *data, unsigned len)
{
    //ESP_LOGI(TAG,"HDLC console data");
    console__hdlc_data(data,len);
}


static void local_uart_init()
{
    hdlc_decoder__init(&console_hdlc_decoder,
                       console_hdlc_buf,
                       sizeof(console_hdlc_buf),
                       &console_hdlc_data,
                       &console_char_data,
                       NULL
                      );
}

static void uart_char(uint8_t c)
{
    hdlc_decoder__append(&console_hdlc_decoder, c);
}


static void  __attribute__((noreturn)) internal_error()
{
    while (1) {
        led__set(LED1, 1);
        vTaskDelay(100 / portTICK_RATE_MS);
        led__set(LED1, 0);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

void app_main(void);

void app_main()
{
    int lstatus = 0;
    int do_restart = 0;

    gpio__init();
    console__init();
    adc__init();
    board__init();

    led__set(LED1, 1);
    led__set(LED2, 0);
    spi__init_bus();
    sdcard__init();
    nvs__init();
    ESP_LOGI(TAG, "Init WSYS");
    wsys__init();

    resource__init();

    ESP_LOGI(TAG, "Init FPGA");

    if (fpga__init()<0) {
        ESP_LOGE(TAG, "Cannot proceed without FPGA!");
        internal_error();
    }

    // Wait for BIT mode detection.
    vTaskDelay(20 / portTICK_RATE_MS);

    // END TEST
    if (fpga__isBITmode()) {
        bit__run();
    }

    if (!extram_test(true)) {
        ESP_LOGE(TAG,"Cannot proceed without external RAM!");
        internal_error();
    }

    // Init local uart for console
    local_uart_init();

    ESP_LOGI(TAG, "Init wifi");

    wifi__init();

    // Set mode if we are using a 2A/3 spectrum;

    if (board__isplus2plus3supply()) {
        // Check if FPGA supports mode
        if ((fpga__id() >>16)!=0xA610) {
            ESP_LOGE(TAG, "Detected +2A/+3 but FPGA binary does not support it (%08x)", fpga__id());
            internal_error();
        }
        fpga__set_clear_flags(FPGA_FLAG_MODE2A, 0);
        spectrum_flags |= SPECTRUM_FLAGS_AY;
        ESP_LOGI(TAG, "Switched to +2A/+3 mode");
    }


    ESP_LOGI(TAG, "Init spectrum");

    
    spectint__init();

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
    vga__init();
    buttons__init();

    ESP_LOGI(TAG, "InterfaceZ version: %s %s", version, gitversion);
    ESP_LOGI(TAG, "  built %s", builddate);
    ESP_LOGI(TAG, " FPGA: 0x%08x", fpga__id());
    ESP_LOGI(TAG, " Spectrum ROM version: %s", *rom__get_version() ? rom__get_version() : "unknown");

    detect_spectrum();

    rom_hook__enable_defaults();

    // Power on USB
    usb_ll__set_power(1);

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
                uart_char(c);
            }
        } while (s==OK);
        // Process buttons
        process_buttons();
    }
}
