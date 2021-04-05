#ifndef __TAP_H__
#define __TAP_H__

#include <inttypes.h>
#include "byteorder.h"

enum tap_state {
    LENGTH,
    DATA
};

struct tap {
    enum tap_state state;
    uint8_t tapbuf[16];
    uint8_t tapbufptr;
    uint16_t datachunk;
    int initial_delay;
};

struct spectrum_tape_header
{
    uint8_t type;      // Type (0,1,2 or 3)
    char filename[10]; // Filename (padded with blanks)
    le_uint16_t length;     // Length of data block
    le_uint16_t param1;     // Parameter 1
    le_uint16_t param2;     // Parameter 2
} __attribute__((packed));

/*
 The type is 0,1,2 or 3 for a Program, Number array, Character array or Code file.
 A SCREEN$ file is regarded as a Code file with start address 16384 and length 6912 decimal.
 If the file is a Program file, parameter 1 holds the autostart line number (or a number >=32768
 if no LINE parameter was given) and parameter 2 holds the start of the variable area relative
 to the start of the program. If it's a Code file, parameter 1 holds the start of the code block
 when saved, and parameter 2 holds 32768. For data files finally, the byte at position 14 decimal
 (second half [MSB] of parameter 1) holds the variable name.

 (originally from TECHINFO.DOC supplied with Z80 by Gerton Lunter)
 */


void tap__standard_block_callback(uint16_t length, uint16_t pause_after);
void tap__init(struct tap *t);
void tap__set_initial_delay(struct tap *t, int initial_delay);
void tap__chunk(struct tap *t, const uint8_t *data, int len);
void tap__finished_callback(void);
void tap__data_finished_callback(void);
void tap__data_callback(const uint8_t *data, int len);

#endif
