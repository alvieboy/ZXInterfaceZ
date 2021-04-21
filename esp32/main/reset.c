#include "reset.h"
#include "fpga.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "log.h"
#include "spectfd.h"

#define TAG "RESET"


/**
 * \defgroup reset Reset handling
 */

static void reset__reset_completed(void);

/**
 * \ingroup reset
 * \brief Return the reset time
 *
 * Return the time, in milisseconds, that ZX Spectrum reset signal must be active to
 * safely perform a reset.
 * \return time in milisseconds
 */

int reset__get_reset_time(void)
{
    return 500;
}

/**
 * \ingroup reset
 * \brief Reset the spectrum
 */
int reset__reset_spectrum(void)
{
    ESP_LOGI(TAG, "Resetting spectrum");

    fpga__set_flags(FPGA_FLAG_RSTSPECT);
    fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_OFF | FPGA_FLAG_TRIG_FORCENMI_OFF);
    vTaskDelay(reset__get_reset_time() / portTICK_RATE_MS);

    reset__reset_completed();

    fpga__clear_flags(FPGA_FLAG_RSTSPECT);

    ESP_LOGI(TAG, "Reset completed");
    return 0;
}

/**
 * \ingroup reset
 * \brief Reset the ZX Spectrum and force it into a custom ROM
 *
 * \param romno ROM number to activate. See ROM architecture details.
 * \param miscctrl Value to be written to MISCCTRL register. This is used to control ROM behaviour
 * \param activate_retn_hook Whether to activate the RETN hook.
 * \return 0 if successful
 */
int reset__reset_to_custom_rom(int romno, uint8_t miscctrl, bool activate_retn_hook)
{
    ESP_LOGI(TAG, "Resetting spectrum (to custom ROM)");

    fpga__set_flags(FPGA_FLAG_RSTSPECT | FPGA_FLAG_CAPCLR);

    fpga__set_rom(romno);

    fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMCS_ON);

    if (activate_retn_hook) {
        fpga__set_trigger(FPGA_FLAG_TRIG_FORCEROMONRETN);
    }

    fpga__write_miscctrl(miscctrl);


    vTaskDelay(reset__get_reset_time() / portTICK_RATE_MS);

    reset__reset_completed();

    fpga__clear_flags(FPGA_FLAG_RSTSPECT);

    ESP_LOGI(TAG, "Reset completed");
    return 0;
}

#include "tapeplayer.h"
#include "fasttap.h"
#include "spectfd.h"
#include "spectdir.h"

static void reset__reset_completed()
{
    tapeplayer__stop();
    fasttap__stop();

    /* Close descriptors associated with spectrum */
    spectfd__close_all();
    spectdir__close_all();
}
