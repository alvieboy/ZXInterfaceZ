#include "fpga.h"
#include "nvs.h"

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

void audio__init()
{
    volumes.v[0].l = nvs__u8("vol_ch0_l", 0xff);
    volumes.v[0].r = nvs__u8("vol_ch0_r", 0xff);
    volumes.v[1].l = nvs__u8("vol_ch1_l", 0xff);
    volumes.v[1].r = nvs__u8("vol_ch1_r", 0xff);
    volumes.v[2].l = nvs__u8("vol_ch2_l", 0xff);
    volumes.v[2].r = nvs__u8("vol_ch2_r", 0xff);
    volumes.v[3].l = nvs__u8("vol_ch3_l", 0xff);
    volumes.v[3].r = nvs__u8("vol_ch3_r", 0xff);

    audio__update();
}

void audio__set_volume( int balance)
{
    // Gain: gL + gR
    // Balance: -63 to 63

}

