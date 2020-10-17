#include <esp_adc_cal.h>

uint32_t esp_adc_cal_raw_to_voltage(uint32_t adc_reading, const esp_adc_cal_characteristics_t *chars)
{
    double ref;
    switch(chars->atten) {
    case ADC_ATTEN_DB_2_5:
        ref = 1100.0F*1.34F;
        break;
    case ADC_ATTEN_DB_6:
        ref = 1100.0F*2.0F;
        break;
    case ADC_ATTEN_DB_11:
        ref = 1100.0F*3.6F;
        break;
    case ADC_ATTEN_DB_0: /* Fall-through */
    default:
        ref = 1100.0F;
        break;
    }
    uint32_t max =  (1<<(9+(unsigned)chars->bit_width))-1;
    return ((double)adc_reading*ref)/(double)max;
}


esp_adc_cal_value_t esp_adc_cal_characterize(adc_unit_t adc_num,
                                             adc_atten_t atten,
                                             adc_bits_width_t bit_width,
                                             uint32_t default_vref,
                                             esp_adc_cal_characteristics_t *chars)
{
    chars->atten = atten;
    chars->bit_width = bit_width;
}
