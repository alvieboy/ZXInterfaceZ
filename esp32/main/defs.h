#ifndef __DEFS_H__
#define __DEFS_H__

#define TAG "ZX InterfaceZ"


#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_QWP  22
#define PIN_NUM_QHD  21
#define PIN_NUM_CS   5

#define SDMMC_PIN_D0 7
#define SDMMC_PIN_D1 8
#define SDMMC_PIN_D2  9
#define SDMMC_PIN_D3  10
#define SDMMC_PIN_CLK 6
#define SDMMC_PIN_CMD 11
//#define SDMMC_PIN_DET 15


#define PIN_NUM_LED1 32
#define PIN_NUM_LED2 32

#define PIN_NUM_IO25 25
#define PIN_NUM_IO26 26
#define PIN_NUM_IO27 27
#define PIN_NUM_IO14 14

#define PIN_NUM_SWITCH 34
#define PIN_NUM_CONF_DONE 15 /* TBD: change to IO15 - SMDMMC_PIN_DET */
#define PIN_NUM_NCONFIG 12

#define PIN_NUM_SPECT_INTERRUPT PIN_NUM_IO26
#define PIN_NUM_AS_CSN PIN_NUM_IO27


#define SPECTRUM_FRAME_SIZE (0x1b00)


#define CMD_PORT 8002
#define BUFFER_PORT 8003


static inline uint32_t getbe32(const uint8_t *source)
{
    uint32_t ret = (((uint32_t)source[0]) << 24) +
        (((uint32_t)source[1]) << 16) +
        (((uint32_t)source[2]) << 8) +
        ((uint32_t)source[3]);
    return ret;
}

#endif
