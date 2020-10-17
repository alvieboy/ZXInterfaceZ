#include "driver/adc.h"

static int adc_chan0_val = 2800;

int adc1_get_raw(adc1_channel_t channel)
{
    switch (channel) {
    case ADC1_CHANNEL_0:
        return adc_chan0_val;
        break;
    default:
        return 0;
    }
}

esp_err_t adc1_config_width(adc_bits_width_t width_bit)
{
    return 0;
}

esp_err_t adc1_config_channel_atten(adc1_channel_t channel, adc_atten_t atten)
{
    return 0;
}
