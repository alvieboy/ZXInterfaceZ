#ifndef __DEFS_H__
#define __DEFS_H__

/**
 * \defgroup misc Miscelaneous
 * \brief Miscelaneous routines and modules
 *
 * \defgroup tools Tools and utilities
 * \brief Tools and utilities
 *
 * \defgroup init System initialization
 * \brief System initialization
 */

#define PIN_NUM_IO26 26
#define PIN_NUM_IO21 21
#define PIN_NUM_IO22 22
#define PIN_NUM_IO27 27
#define PIN_NUM_IO0  0
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18

//#define PIN_NUM_QWP  22
//#define PIN_NUM_QHD  21
#define PIN_NUM_CS   5
#define PIN_NUM_CS2  PIN_NUM_IO22


#define SDMMC_PIN_D0 2
#define SDMMC_PIN_D1 4
#define SDMMC_PIN_D2  12
#define SDMMC_PIN_D3  13
#define SDMMC_PIN_CLK 14
#define SDMMC_PIN_CMD 15
//#define SDMMC_PIN_DET 15


#define PIN_NUM_LED1 32
#define PIN_NUM_PULLUP_EN 33


#define PIN_NUM_SWITCH 34


// FPGA-configuration pins
#define PIN_NUM_CONF_DONE 35 
#define PIN_NUM_NCONFIG 25
#define PIN_NUM_NSTATUS 16

#define PIN_NUM_CMD_INTERRUPT PIN_NUM_IO27

#define PIN_NUM_SPECT_INTERRUPT PIN_NUM_IO26
#define PIN_NUM_INTACK        PIN_NUM_IO21

#define SPECTRUM_FRAME_SIZE (0x1b00)


#define CMD_PORT 8002
#define BUFFER_PORT 8003

#define INTERFACEZ_MAX_FILES_PER_MOUNT 2

void request_restart(void);

#ifndef __linux__
#include <esp_heap_caps.h>
#define HEAPCHECK(xpto...) do { \
    if (!heap_caps_check_integrity(0xffffffff, true)) { \
    ESP_LOGE("HEAP","Heap check fail %s line %d", __PRETTY_FUNCTION__, __LINE__); \
    abort(); \
    } \
} while (0)
#else
#define HEAPCHECK(x...)

#endif


#endif
