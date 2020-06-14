#ifndef __TZX_H__
#define __TZX_H__

#include <inttypes.h>

enum tzx_state
{
    HEADER,
    BLOCK,
    DESCRIPTION,
    STANDARD,
    TURBOSPEED,
    RAWDATA,
    INVALID
};

struct tzx {
    uint8_t tzxbuf[256];
    uint8_t tzxbufptr;
    uint8_t lastbytesize;
    enum tzx_state state;
    uint32_t datachunk;
};


void tzx__init(struct tzx *t);
void tzx__chunk(struct tzx *t, const uint8_t *data, int len);
void tzx__standard_block_callback(uint16_t length, uint16_t pause_after);
void tzx__turbo_block_callback(uint16_t pilot, uint16_t sync0, uint16_t sync1, uint16_t pulse0, uint16_t pulse1, uint16_t data_len);
void tzx__data_callback(const uint8_t *data, int len);

#endif