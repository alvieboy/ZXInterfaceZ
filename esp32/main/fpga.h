#ifndef __FPGA_H__
#define __FPGA_H__


#include <inttypes.h>
#include "esp_system.h"
#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t fpga_flags_t;
typedef uint8_t fpga_status_t;

#define FPGA_SPI_CMD_READ_STATUS (0xDE)
#define FPGA_SPI_CMD_READ_VIDEOMEM (0xDF)
#define FPGA_SPI_CMD_READ_CAPMEM (0xE0)
#define FPGA_SPI_CMD_WRITE_ROM (0xE1)
#define FPGA_SPI_CMD_READ_CAPSTATUS (0xE2)
#define FPGA_SPI_CMD_WRITE_RESOURCEFIFO (0xE3)
#define FPGA_SPI_CMD_WRITE_TAPFIFO (0xE4)
#define FPGA_SPI_CMD_READ_TAPFIFO_USAGE (0xE5)
#define FPGA_SPI_CMD_WRITE_TAPCMD (0xE6)
#define FPGA_SPI_CMD_WRITE_FLAGS (0xEC)
#define FPGA_SPI_CMD_READ_REG32 (0xEE)
#define FPGA_SPI_CMD_WRITE_REG32 (0xED)
#define FPGA_SPI_CMD_SETEOF (0xEF)
#define FPGA_SPI_CMD_READ_DATAFIFO (0xFC)
#define FPGA_SPI_CMD_READ_CMDFIFO (0xFB)
#define FPGA_SPI_CMD_READ_ID (0x9E)
#define FPGA_SPI_CMD_READ_PC (0x40)
#define FPGA_SPI_CMD_READ_EXTRAM (0x50)
#define FPGA_SPI_CMD_WRITE_EXTRAM (0x51)
#define FPGA_SPI_CMD_READ_USB (0x60)
#define FPGA_SPI_CMD_WRITE_USB (0x61)
#define FPGA_SPI_CMD_SET_ROMRAM (0xEB)
#define FPGA_SPI_CMD_READ_UART_STATUS (0xDA)
#define FPGA_SPI_CMD_WRITE_UART_DATA (0xD8)
#define FPGA_SPI_CMD_READ_UART_DATA (0xD9)
#define FPGA_SPI_CMD_READ_BIT (0xD7)
#define FPGA_SPI_CMD_WRITE_BIT (0xD6)

/* Status bits */
#define FPGA_STATUS_RESFIFO_FULL   (1<<1)
#define FPGA_STATUS_RESFIFO_QFULL  (1<<2)
#define FPGA_STATUS_RESFIFO_HFULL  (1<<3)
#define FPGA_STATUS_RESFIFO_QQQFULL  (1<<4)
#define FPGA_STATUS_BITMODE_REQUESTED  (1<<7)
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
#define FPGA_FLAG_MODE2A (1<<13)
#define FPGA_FLAG_BITMODE (1<<14)



#define FPGA_FLAG_TRIG_RESOURCEFIFO_RESET (1<<0)
#define FPGA_FLAG_TRIG_FORCEROMONRETN     (1<<1)
#define FPGA_FLAG_TRIG_FORCEROMCS_ON      (1<<2)
#define FPGA_FLAG_TRIG_FORCEROMCS_OFF     (1<<3)
#define FPGA_FLAG_TRIG_INTACK             (1<<4)
#define FPGA_FLAG_TRIG_CMDFIFO_RESET      (1<<5)
#define FPGA_FLAG_TRIG_FORCENMI_ON        (1<<6)
#define FPGA_FLAG_TRIG_FORCENMI_OFF       (1<<7)

/* UART BIT */
#define FPGA_UART_STATUS_BUSY (1<<0)
#define FPGA_UART_STATUS_RX_AVAIL (1<<1)

/* Registers */

#define REG_NMIREASON    0x00
#define REG_CONFIG1      0x02
# define CONFIG1_KBD_ENABLE (1<<0)
# define CONFIG1_JOY_ENABLE (1<<1)
# define CONFIG1_MOUSE_ENABLE (1<<2)
# define CONFIG1_AY_ENABLE (1<<3)
# define CONFIG1_AY_READ_ENABLE (1<<4)
#define REG_KEYB1_DATA      0x03
#define REG_KEYB2_DATA      0x04
#define REG_JOY_DATA        0x05
#define REG_MOUSE_DATA      0x05 /* Same as joy */
#define REG_VOLUME(x)       (0x06+(x))

#define FPGA_RESOURCE_FIFO_SIZE 1024 /* Should be 1024 */
#define FPGA_TAP_FIFO_SIZE 1023 /* Should be 1024 */

#define ROM_0 0
#define ROM_1 1
#define ROM_2 2


int fpga__init(void);

void fpga__set_clear_flags(fpga_flags_t enable, fpga_flags_t disable);
void fpga__set_trigger(uint8_t trig);
void fpga__get_framebuffer(uint8_t *target);
void fpga__set_register(uint8_t reg, uint32_t value);
uint32_t fpga__get_register(uint8_t reg);
uint32_t fpga__id(void);
void fpga__set_capture_mask(uint32_t mask);
void fpga__set_capture_value(uint32_t value);
int fpga__get_captures(uint8_t *target);
int fpga__upload_rom_chunk(const uint32_t baseaddress, uint16_t offset, uint8_t *buffer_sub3, unsigned len);
int fpga__upload_rom(const uint32_t baseaddress, const uint8_t *buffer, unsigned len);

int fpga__isBITmode(void);

int fpga__reset_to_custom_rom(int romno, bool activate_retn_hook);

int fpga__load_resource_fifo(const uint8_t *data, unsigned len, int timeout);


int fpga__passiveserialconfigure(const uint8_t *data, unsigned len);
int fpga__passiveserialconfigure_fromfile(int fh, unsigned len);

#if 0
int fpga__startprogram(fpga_program_state_t*);
int fpga__program(fpga_program_state_t*,const uint8_t *data, unsigned len);
int fpga__finishprogram(fpga_program_state_t*);
void fpga__trigger_reconfiguration(void);
#endif

uint8_t fpga__get_status(void);

static inline void fpga__set_flags(fpga_flags_t enable)
{
    fpga__set_clear_flags(enable, 0);
}
static inline void fpga__clear_flags(fpga_flags_t disable)
{
    fpga__set_clear_flags(0, disable);
}
uint32_t fpga__get_capture_status(void);
int fpga__read_command_fifo(void);
uint16_t fpga__get_spectrum_pc(void);
int fpga__load_tap_fifo(const uint8_t *data, unsigned len, int timeout);
int fpga__load_tap_fifo_command(const uint8_t *data, unsigned len, int timeout);
uint16_t fpga__get_tap_fifo_free(void);
bool fpga__tap_fifo_empty(void);

int fpga__write_rom(unsigned offset, uint8_t val);

/* EXT ram methods */
int fpga__read_extram(uint32_t address);
int fpga__read_extram_block(uint32_t address, uint8_t *dest, int size);
int fpga__write_extram(uint32_t address, uint8_t val);
int fpga__write_extram_block(uint32_t address, const uint8_t *buffer, int size);
int fpga__write_extram_block_from_file(uint32_t address, int fd, int size, bool verify);


int fpga__read_usb(uint16_t address);
int fpga__read_usb_block(uint16_t address, uint8_t *dest, int size);
int fpga__write_usb(uint16_t address, uint8_t val);
int fpga__write_usb_block(uint16_t address, const uint8_t *buffer, int size);

void fpga__set_config1_bits(uint32_t bits);
void fpga__clear_config1_bits(uint32_t bits);
int fpga__set_rom(uint8_t rom);
int fpga__set_ram(uint8_t ram);
int fpga__reset_spectrum(void);

int fpga__read_uart_status(void);
int fpga__read_uart_data(uint8_t *buf, int len);
int fpga__write_uart_data(uint8_t);
int fpga__write_bit_data(const uint8_t *data, unsigned len);
int fpga__read_bit_data(uint8_t *data, unsigned len);

#ifdef __cplusplus
}
#endif

#endif
