#ifndef __SAVE_H__
#define __SAVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>

void save__notify_save_to_tap(void);
void save__notify_no_save(void);
void save__set_data_from_header(const uint8_t *data, unsigned size);

const char *save__get_requested_name();

int save__start_save_tap(const char *filename,
                         bool append);

void save__start_save_physical(void);

int save__append_from_extram(uint32_t address, uint16_t datalen);
void save__stop_tape();

#ifdef __cplusplus
}
#endif


#endif

