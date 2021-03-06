/**
 * \defgroup audio Audio routines
 * \brief Audio configuration
 *
 * These routines configure the audio subsystem volumes.
 *
 * Across all functions, a floating point volume should be between 0.0 and 2.0. The nominal volume is 1.0.
 * The Balance (left-right) should be between -1.0 (full left) and +1.0 (full right), being 0.0 center.
 *
 */
#include "fpga.h"
#include "onchip_nvs.h"
#include <math.h>
#include "esp_log.h"
#include "audio.h"

typedef union {
    struct {
        uint8_t l;
        uint8_t r;
    };
    uint16_t vol;
} chan_volume_t;

static union {
    chan_volume_t v[4];
    struct {
        uint32_t ch0_ch1;
        uint32_t ch2_ch3;
    };
} volumes;

static void audio__update()
{
    fpga__set_register(REG_VOLUME(0), volumes.ch0_ch1);
    fpga__set_register(REG_VOLUME(1), volumes.ch2_ch3);
}

/*
 Balance:
 -1.0 : left channel
 0.0  : both channels
 +1.0 : right channel
*/

static void audio__balance_to_channels(float volume,
                                       float balance,
                                       uint8_t *l,
                                       uint8_t *r)
{
    if (balance>1.0F)
        balance = 1.0F;
    if (balance<-1.0F)
        balance = -1.0F;
    if (volume>2.0F)
        volume = 2.0F;

    float p, gl, gr;

    p = (float)(M_PI*(balance+1)/4.0F);

    gl = cosf(p)*volume;
    gr = sinf(p)*volume;

    unsigned left = (unsigned)(255.0F * gl);

    unsigned right = (unsigned)(255.0F * gr);

    if (left>255)
        left=255;
    if (right>255)
        right=255;

    *l = (uint8_t)left;
    *r = (uint8_t)right;
}

static void audio__balance_to_channel(uint8_t chan,
                                      float volume,
                                      float balance,
                                      chan_volume_t *vols)
{
    audio__balance_to_channels(volume,
                               balance,
                               &vols->l,
                               &vols->r);
    ESP_LOGI("AUDIO",
             "Ch%d: Volume %f balance %f -> l=%d r=%d",
             chan, volume, balance,
             vols->l, vols->r
            );
}

/**
 * \ingroup audio
 * \brief Initialise the audio subsystem
 */
void audio__init()
{
    uint8_t i;
    char vol_key[8] = "vol_chX";
    char bal_key[8] = "bal_chX";
    for (i=0; i<4; i++) {
        vol_key[6] = (char)('0' + i);
        bal_key[6] = (char)('0' + i);
        audio__balance_to_channel(i,
                                  nvs__float(vol_key, 1.0F),
                                  nvs__float(bal_key, 0.0F),
                                  &volumes.v[i]
                             );
    }
    audio__update();
    fpga__set_clear_flags(FPGA_FLAG_ENABLE_AUDIO,0);
}

/**
 * \ingroup audio
 * \brief Set volume and balance for a specific channel (floating point)
 *
 * Sets the current volume and balance for a specific channel.
 *
 * \param chan The audio channel. See audio channel defines for details
 * \param volume Volume to be applied. 
 * \param balance Balance (left-right) to be applied. 
 *
 *
 */
void audio__set_volume_f(uint8_t chan, float volume, float balance)
{
    char vol_key[8] = "vol_chX";
    char bal_key[8] = "bal_chX";

    vol_key[6] = (char)('0' + chan);
    bal_key[6] = (char)('0' + chan);

    audio__balance_to_channel(chan,
                              volume,
                              balance,
                              &volumes.v[chan]
                             );

    nvs__set_float(vol_key, volume);
    nvs__set_float(bal_key, balance);

    audio__update();
    nvs__commit();

}

/**
 * \ingroup audio
 * \brief Return current volume and balance for a specific channel (floating point)
 *
 * Returns the current volume and balance for a specific channel.
 *
 * \param chan The audio channel. See audio channel defines for details
 * \param volume Pointer to variable where volume will be stored
 * \param balance Pointer to variable where balance will be stored
 *
 *
 */
void audio__get_volume_f(uint8_t chan, float *volume, float *balance)
{
    char vol_key[8] = "vol_chX";
    char bal_key[8] = "bal_chX";

    vol_key[6] = (char)('0' + chan);
    bal_key[6] = (char)('0' + chan);

    *volume = nvs__float(vol_key, 1.0F);
    *balance = nvs__float(bal_key, 0.0F);
}
