#include "adc.h"

#include <inttypes.h>
#include <esp_adc_cal.h>

#define DEFAULT_VREF 1100
static esp_adc_cal_characteristics_t adc_characteristics;

void adc__init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);

    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, &adc_characteristics);
}

uint32_t adc__read_v()
{
    uint32_t val = adc1_get_raw(ADC1_CHANNEL_0);
    uint32_t milivolts = esp_adc_cal_raw_to_voltage(val, &adc_characteristics);
    return milivolts;
}
