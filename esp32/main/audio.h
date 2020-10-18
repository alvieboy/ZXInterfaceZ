#ifndef __AUDIO_H__
#define __AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_CHAN_BEEPER 0
#define AUDIO_CHAN_AY_CH0 1
#define AUDIO_CHAN_AY_CH1 2
#define AUDIO_CHAN_AY_CH2 3

void audio__init(void);
void audio__set_volume_f(uint8_t chan, float volume, float balance);
void audio__get_volume_f(uint8_t chan, float *volume, float *balance);

#ifdef __cplusplus
}
#endif

#endif
