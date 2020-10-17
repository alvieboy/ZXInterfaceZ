#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C"  {
#endif

#include <inttypes.h>

void adc__init(void);
uint32_t adc__read_v(void);

#ifdef __cplusplus
}
#endif

#endif
