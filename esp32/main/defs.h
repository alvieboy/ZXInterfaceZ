#ifndef __DEFS_H__
#define __DEFS_H__

#define TAG "ZX InterfaceZ"


#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_QWP  22
#define PIN_NUM_QHD  21
#define PIN_NUM_CS   5

#define SDMMC_PIN_D0 2
#define SDMMC_PIN_D1 4
#define SDMMC_PIN_D2  12
#define SDMMC_PIN_D3  13
#define SDMMC_PIN_CLK 14
#define SDMMC_PIN_CMD 15
//#define SDMMC_PIN_DET 15


#define PIN_NUM_LED1 32
#define PIN_NUM_PULLUP_EN 33

#define PIN_NUM_IO26 26
#define PIN_NUM_IO27 27

#define PIN_NUM_SWITCH 34


// FPGA-configuration pins
#define PIN_NUM_CONF_DONE 35 
#define PIN_NUM_NCONFIG 25
#define PIN_NUM_NSTATUS 16

#define PIN_NUM_CMD_INTERRUPT PIN_NUM_IO27

#define PIN_NUM_SPECT_INTERRUPT PIN_NUM_IO26


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

static inline uint16_t getbe16(const uint8_t *source)
{
    uint16_t ret = ((((uint16_t)source[0]) << 8) +
        ((uint16_t)source[1]));
    return ret;
}

void request_restart();


#endif
