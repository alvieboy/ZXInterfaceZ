#ifndef __SCOPE_H__
#define __SCOPE_H__

#include <inttypes.h>

#define SCOPE_WIDTH_BITS 10

#define SCOPE_CAPTURE_LEN (((1<<SCOPE_WIDTH_BITS)-1) & ~0xF)

void scope__start(uint8_t clockdiv);
void scope__set_triggers(uint32_t mask,
                         uint32_t value,
                         uint32_t edge);

void scope__get_capture_info(uint32_t *status, uint32_t *counter, uint32_t *trig_address);
// Get 64 samples (256 bytes).
void scope__get_capture_data_block64(uint8_t channel, uint8_t offset, uint8_t*dest);

#endif
