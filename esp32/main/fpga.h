#ifndef __FPGA_H__
#define __FPGA_H__

#include <inttypes.h>
#include "esp_system.h"

typedef uint16_t fpga_flags_t;
typedef uint8_t fpga_status_t;

typedef struct
{
    int first_unerased_address;
    int current_write_address;
    unsigned flashid;
    //uint8_t page[FLASH_PAGE_SIZE];
} fpga_program_state_t;

#define FPGA_SPI_CMD_READ_STATUS (0xDE)
#define FPGA_SPI_CMD_READ_VIDEOMEM (0xDF)
#define FPGA_SPI_CMD_READ_CAPMEM (0xE0)
#define FPGA_SPI_CMD_WRITE_ROM (0xE1)
#define FPGA_SPI_CMD_READ_CAPSTATUS (0xE2)
#define FPGA_SPI_CMD_WRITE_RESOURCEFIFO (0xE3)
#define FPGA_SPI_CMD_WRITE_TAPFIFO (0xE4)
#define FPGA_SPI_CMD_READ_TAPFIFO_USAGE (0xE5)
#define FPGA_SPI_CMD_WRITE_FLAGS (0xEC)
#define FPGA_SPI_CMD_WRITE_REG32 (0xEE)
#define FPGA_SPI_CMD_READ_REG32 (0xED)
#define FPGA_SPI_CMD_SETEOF (0xEF)
#define FPGA_SPI_CMD_READ_DATAFIFO (0xFC)
#define FPGA_SPI_CMD_READ_ID (0x9E)
/* Status bits */
#define FPGA_STATUS_DATAFIFO_EMPTY (1<<0)
#define FPGA_STATUS_RESFIFO_FULL   (1<<1)
#define FPGA_STATUS_RESFIFO_QFULL  (1<<2)
#define FPGA_STATUS_RESFIFO_HFULL  (1<<3)
#define FPGA_STATUS_RESFIFO_QQQFULL  (1<<4)

/* Flags */

#define FPGA_FLAG_RSTFIFO (1<<0)
#define FPGA_FLAG_RSTSPECT (1<<1)
#define FPGA_FLAG_CAPCLR (1<<2)
#define FPGA_FLAG_CAPRUN (1<<3)
#define FPGA_FLAG_COMPRESS (1<<4)
#define FPGA_FLAG_ENABLE_INTERRUPT (1<<5)
#define FPGA_FLAG_CAPSYNCEN (1<<6)

#define FPGA_FLAG_ULAHACK (1<<8)
#define FPGA_FLAG_TAPFIFO_RESET (1<<9)
#define FPGA_FLAG_TAP_ENABLED (1<<10)

#define FPGA_FLAG_VIDMODE0 (1<<11)
#define FPGA_FLAG_VIDMODE1 (1<<12)



#define FPGA_FLAG_TRIG_RESOURCEFIFO_RESET (1<<0)
#define FPGA_FLAG_TRIG_FORCEROMONRETN     (1<<1)
#define FPGA_FLAG_TRIG_FORCEROMCS_ON      (1<<2)
#define FPGA_FLAG_TRIG_FORCEROMCS_OFF     (1<<3)
#define FPGA_FLAG_TRIG_INTACK             (1<<4)
#define FPGA_FLAG_TRIG_CMDFIFO_RESET      (1<<5)
#define FPGA_FLAG_TRIG_FORCENMI_ON        (1<<6)
#define FPGA_FLAG_TRIG_FORCENMI_OFF       (1<<7)

/* Registers */

#define REG_CAPTURE_MASK 0x0
#define REG_CAPTURE_VAL 0x1

#define FPGA_RESOURCE_FIFO_SIZE 1024 /* Should be 1024 */
#define FPGA_TAP_FIFO_SIZE 1023 /* Should be 1024 */

int fpga__init(void);

void fpga__set_clear_flags(fpga_flags_t enable, fpga_flags_t disable);
void fpga__set_trigger(uint8_t trig);
void fpga__get_framebuffer(uint8_t *target);
void fpga__set_register(uint8_t reg, uint32_t value);
uint32_t fpga__get_register(uint8_t reg);
uint32_t fpga__read_id(void);
void fpga__set_capture_mask(uint32_t mask);
void fpga__set_capture_value(uint32_t value);
int fpga__get_captures(uint8_t *target);
int fpga__upload_rom_chunk(uint16_t offset, uint8_t *buffer_sub3, unsigned len);
int fpga__upload_rom(const uint8_t *buffer, unsigned len);


int fpga__reset_to_custom_rom(bool activate_retn_hook);

int fpga__load_resource_fifo(const uint8_t *data, unsigned len, int timeout);


int fpga__passiveserialconfigure(const uint8_t *data, unsigned len);
#if 0
int fpga__startprogram(fpga_program_state_t*);
int fpga__program(fpga_program_state_t*,const uint8_t *data, unsigned len);
int fpga__finishprogram(fpga_program_state_t*);
void fpga__trigger_reconfiguration(void);
#endif

uint8_t fpga__get_status();

static inline void fpga__set_flags(fpga_flags_t enable)
{
    fpga__set_clear_flags(enable, 0);
}
static inline void fpga__clear_flags(fpga_flags_t disable)
{
    fpga__set_clear_flags(0, disable);
}
uint32_t fpga__get_capture_status(void);
int fpga__read_command_fifo();
uint16_t fpga__get_spectrum_pc();
int fpga__load_tap_fifo(const uint8_t *data, unsigned len, int timeout);
uint16_t fpga__get_tap_fifo_free();
bool fpga__tap_fifo_empty();

int fpga__write_rom(unsigned offset, uint8_t val);

/* EXT ram methods */
int fpga__read_extram(uint32_t address);
int fpga__read_extram_block(uint32_t address, uint8_t *dest, int size);
int fpga__write_extram(uint32_t address, uint8_t val);
int fpga__write_extram_block(uint32_t address, uint8_t *buffer, int size);


int fpga__read_usb(uint16_t address);
int fpga__read_usb_block(uint16_t address, uint8_t *dest, int size);
int fpga__write_usb(uint16_t address, uint8_t val);
int fpga__write_usb_block(uint16_t address, const uint8_t *buffer, int size);


#endif