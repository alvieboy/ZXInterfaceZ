#ifndef __TAP_H__
#define __TAP_H__

#include <inttypes.h>

enum tap_state {
    LENGTH,
    DATA
};

struct tap {
    enum tap_state state;
    uint8_t tapbuf[16];
    uint8_t tapbufptr;
    uint16_t datachunk;
};


void tap__standard_block_callback(uint16_t length, uint16_t pause_after);
void tap__init(struct tap *t);
void tap__chunk(struct tap *t, const uint8_t *data, int len);
void tap__finished_callback(void);
void tap__data_finished_callback(void);
void tap__data_callback(const uint8_t *data, int len);

#endif
