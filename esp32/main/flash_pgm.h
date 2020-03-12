#ifndef __FLASHPGM_H__
#define __FLASHPGM_H__

void flash_pgm__init(void);

unsigned int fpga_pgm__read_id();
int flash_pgm__erase_sector_address(int sector_address);
int flash_pgm__program_page( int address, const uint8_t *data, int size);



#endif
