/**
 * \defgroup adc ADC operations
 * \brief Routines to manipulate the ESP32 ADC
 *
 * adc__init() should be called before any read operation is performed on the ADC.
 *
 * After initialization, adc__read_v() can be used to read the current voltage
 * present at the ADC pins.
 *
 * These methods are used to detect the ZX Spectrum model type (if it runs under 9V or 12V)
 */

#include "adc.h"
#include <inttypes.h>
#include <esp_adc_cal.h>

#define DEFAULT_VREF 1100
static esp_adc_cal_characteristics_t adc_characteristics;

/**
 * \ingroup adc
 * \brief Initialise the ADC system
 *
 * This method should be called before reading any ADC value.
 */
void adc__init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);

    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, &adc_characteristics);
}

/**
 * \ingroup adc
 * \brief Read the current ADC voltage.
 *
 *
 * \return The number of millivolts present ar the ADC input.
 */
uint32_t adc__read_v()
{
    uint32_t val = adc1_get_raw(ADC1_CHANNEL_0);
    uint32_t milivolts = esp_adc_cal_raw_to_voltage(val, &adc_characteristics);
    return milivolts;
}
