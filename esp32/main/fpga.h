#ifndef __FPGA_H__
#define __FPGA_H__


#include <inttypes.h>
#include "esp_system.h"
#include "esp_partition.h"
#include "union_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t fpga_flags_t;
typedef uint8_t fpga_status_t;

struct stream;

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
#define FPGA_SPI_CMD_READ_REG32 (0xED)
#define FPGA_SPI_CMD_WRITE_REG32 (0xEE)
#define FPGA_SPI_CMD_SETEOF (0xEF)
#define FPGA_SPI_CMD_READ_CMDFIFO (0xFB)
#define FPGA_SPI_CMD_WRITE_MISCCTRL (0xFC)
#define FPGA_SPI_CMD_READ_MICIDLE (0xFD)
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
#define FPGA_SPI_CMD_READ_CAP (0x62)
#define FPGA_SPI_CMD_WRITE_CAP (0x63)
#define FPGA_SPI_CMD_READ_CTRL (0x64)
#define FPGA_SPI_CMD_WRITE_CTRL (0x65)
#define FPGA_SPI_CMD_READ_INTERRUPT_ACK (0xA0)
#define FPGA_SPI_CMD_READ_INTERRUPT_STATUS (0xA1)

/* Status bits */
#define FPGA_STATUS_RESFIFO_FULL   (1<<0)
#define FPGA_STATUS_RESFIFO_QFULL  (1<<1)
#define FPGA_STATUS_RESFIFO_HFULL  (1<<2)
#define FPGA_STATUS_RESFIFO_QQQFULL  (1<<3)
#define FPGA_STATUS_CMDFIFO_USED(x)  (((x)>>4)&0x7)  // bits 4, 5, 6
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
#define FPGA_FLAG_ENABLE_AUDIO (1<<15)


/**
 \addtogroup fpga
 \brief FPGA Triggers
 @{ */

/** \brief Reset resource FIFO */
#define FPGA_FLAG_TRIG_RESOURCEFIFO_RESET (1<<0) 
/** \brief Force regular ROM selection upon RETN detection */
#define FPGA_FLAG_TRIG_FORCEROMONRETN     (1<<1)
/** \brief Force ROMCS to ON */
#define FPGA_FLAG_TRIG_FORCEROMCS_ON      (1<<2)
/** \brief Force ROMCS to OFF */
#define FPGA_FLAG_TRIG_FORCEROMCS_OFF     (1<<3)
/** \brief Acknowledge interrupt */
#define FPGA_FLAG_TRIG_INTACK             (1<<4)
/** \brief Reset the command FIFO */
#define FPGA_FLAG_TRIG_CMDFIFO_RESET      (1<<5)
/** \brief Force NMI signal ON */
#define FPGA_FLAG_TRIG_FORCENMI_ON        (1<<6)
/** \brief Force NMI signal OFF */
#define FPGA_FLAG_TRIG_FORCENMI_OFF       (1<<7)
/** @} */

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
# define CONFIG1_DIVMMC_COMPAT (1<<5)
#define REG_KEYB1_DATA      0x03
#define REG_KEYB2_DATA      0x04
#define REG_KEMPSTON        0x05 
#define REG_JOY_DATA        0x05 /* Same as kempston */
#define REG_MOUSE_DATA      0x05 /* Same as kempston */

#define REG_VOLUME(x)       (0x06+(x))

#define FPGA_RESOURCE_FIFO_SIZE 1024 /* Should be 1024 */
#define FPGA_TAP_FIFO_SIZE 1023 /* Should be 1024 */

/* Interrupts */
#define FPGA_INTERRUPT_CMD (1<<0)
#define FPGA_INTERRUPT_USB (1<<1)
#define FPGA_INTERRUPT_SPECT (1<<2)

#define ROM_0 0
#define ROM_1 1
#define ROM_2 2


int fpga__init(void);

void fpga__set_clear_flags(fpga_flags_t enable, fpga_flags_t disable);
void fpga__set_trigger(uint8_t trig);
void fpga__get_framebuffer(uint32_t *target);
void fpga__set_register(uint8_t reg, uint32_t value);
uint32_t fpga__get_register(uint8_t reg);
uint32_t fpga__id(void);
void fpga__set_capture_mask(uint32_t mask);
void fpga__set_capture_value(uint32_t value);
int fpga__get_captures(uint8_t *target);
int fpga__upload_rom_chunk(const uint32_t baseaddress, uint16_t offset, uint8_t *buffer_sub3, unsigned len);
int fpga__upload_rom(const uint32_t baseaddress, const uint8_t *buffer, unsigned len);

int fpga__isBITmode(void);

int fpga__reset_to_custom_rom(int romno, uint8_t miscctrl, bool activate_retn_hook);

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
int fpga__read_command_fifo(uint32_t *dest);
uint16_t fpga__get_spectrum_pc(void);
int fpga__load_tap_fifo(const uint8_t *data, unsigned len, int timeout);
int fpga__load_tap_fifo_command(const uint8_t *data, unsigned len, int timeout);
uint16_t fpga__get_tap_fifo_free(void);
bool fpga__tap_fifo_empty(void);

int fpga__write_rom(unsigned offset, uint8_t val);

/* EXT ram methods */
int fpga__read_extram(uint32_t address);
int fpga__read_extram_block(uint32_t address, uint32_t *dest, int size_bytes);
int fpga__write_extram(uint32_t address, uint8_t val);
int fpga__write_extram_block(uint32_t address, const uint8_t *buffer, int size);
int fpga__write_extram_block_from_file(uint32_t address, int fd, int size);
int fpga__write_extram_block_from_stream(uint32_t address, struct stream *, int size);
int fpga__read_extram_block_into_file(uint32_t address, int fd, int size, uint8_t *checksum);


int fpga__read_usb(uint16_t address);
int fpga__read_usb_block(uint16_t address, uint32_t *dest, int size);
int fpga__write_usb(uint16_t address, uint8_t val);
int fpga__write_usb_block(uint16_t address, const uint8_t *buffer, int size);

void fpga__set_config1_bits(uint32_t bits);
void fpga__clear_config1_bits(uint32_t bits);
int fpga__set_rom(uint8_t rom);
int fpga__set_ram(uint8_t ram);
int fpga__reset_spectrum(void);
int fpga__get_reset_time(void);

int fpga__read_uart_status(void);
int fpga__read_uart_data(uint32_t *buf, int len_bytes);
int fpga__write_uart_data(uint8_t);
int fpga__write_bit_data(const uint8_t *data, unsigned len);
int fpga__read_bit_data(uint32_t *data, unsigned len_bytes);

int fpga__read_capture_block(uint16_t address, uint32_t *dest, int size_bytes);
int fpga__write_capture_block(uint16_t address, const uint8_t *buffer, int size);

int fpga__write_hook(uint8_t index, uint16_t start, uint8_t len, uint8_t flag);
int fpga__disable_hook(uint8_t index);
int fpga__read_hooks(uint32_t *dest);

int fpga__write_miscctrl(uint8_t value);
int fpga__read_mic_idle(void);

//int fpga__disable_hooks(void);

//#define HOOK_ROM1(address) ((address)|(1<<(6+8)))
//#define HOOK_ROM0(address) ((address))

//int fpga__enable_hook(uint8_t index, uint16_t start, uint8_t len);

int fpga__readinterrupt(void);
int fpga__ackinterrupt(uint8_t mask);

int fpga__write_extram_block_from_file_nonblock(uint32_t address, int fd, int size,
                                                int *writtensize);

#ifdef __cplusplus
}
#endif

#endif
