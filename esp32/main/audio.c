#include "fpga.h"
#include "nvs.h"
#include <math.h>
#include "esp_log.h"

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

    p=M_PI*(balance+1)/4;

    gl = cos(p)*volume;
    gr = sin(p)*volume;

    unsigned left = 255.0F * gl;

    unsigned right = 255.0F * gr;

    if (left>255)
        left=255;
    if (right>255)
        right=255;

    *l = left;
    *r = right;
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


void audio__init()
{
    audio__balance_to_channel(0,
                              nvs__float("vol_ch0", 1.0F),
                              nvs__float("bal_ch0", 0.0F),
                              &volumes.v[0]
                             );
    audio__balance_to_channel(1,
                              nvs__float("vol_ch1", 1.0F),
                              nvs__float("bal_ch1", 0.0F),
                              &volumes.v[1]
                             );
    audio__balance_to_channel(2,
                              nvs__float("vol_ch2", 1.0F),
                              nvs__float("bal_ch2", 0.0F),
                              &volumes.v[2]
                             );
    audio__balance_to_channel(3,
                              nvs__float("vol_ch3", 1.0F),
                              nvs__float("bal_ch3", 0.0F),
                              &volumes.v[3]
                             );

    audio__update();
}

void audio__set_volume( int balance)
{
    // Gain: gL + gR
    // Balance: -63 to 63

}

