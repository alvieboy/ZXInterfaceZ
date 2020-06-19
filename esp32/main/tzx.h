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
    IGNOREDATA,
    ARCHIVEINFO,
    STRING,
    GROUP,
    PURETONE,
    PULSES,
    PULSEDATA,
    PUREDATA,
    INVALID
};

struct tzx {
    uint8_t tzxbuf[32];
    uint16_t pulse_data[256];
    uint8_t tzxbufptr;
    uint8_t lastbytesize;
    enum tzx_state state;
    uint8_t pulsecount;
    uint8_t pulses;
    uint32_t datachunk;
};


void tzx__init(struct tzx *t);
void tzx__chunk(struct tzx *t, const uint8_t *data, int len);
void tzx__standard_block_callback(uint16_t length, uint16_t pause_after);
void tzx__turbo_block_callback(uint16_t pilot, uint16_t sync0, uint16_t sync1, uint16_t pulse0, uint16_t pulse1,
                               uint16_t pilot_len, uint16_t gap_len, uint32_t data_len,
                               uint8_t last_byte_len);
void tzx__data_callback(const uint8_t *data, int len);
void tzx__tone_callback(uint16_t t_states, uint16_t count);
void tzx__pulse_callback(uint8_t count, const uint16_t *states);
void tzx__pure_data_callback(uint16_t pulse0, uint16_t pulse1, uint32_t data_len, uint16_t gap,
                             uint8_t last_byte_len);

void tzx__data_finished_callback(void);
void tzx__finished_callback(void);

#endif
