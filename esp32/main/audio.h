#ifndef __AUDIO_H__
#define __AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 *\ingroup audio
 * @{
 */

/** Beeper channel */
#define AUDIO_CHAN_BEEPER 0
/** AY Channel 0 channel */
#define AUDIO_CHAN_AY_CH0 1
/** AY Channel 1 channel */
#define AUDIO_CHAN_AY_CH1 2
/** AY Channel 2 channel */
#define AUDIO_CHAN_AY_CH2 3

/** @} */

void audio__init(void);
void audio__set_volume_f(uint8_t chan, float volume, float balance);
void audio__get_volume_f(uint8_t chan, float *volume, float *balance);


#ifdef __cplusplus
}
#endif

#endif
