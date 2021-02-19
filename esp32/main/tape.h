#ifndef __TAPE_H__
#define __TAPE_H__

#include <stdbool.h>
#include <inttypes.h>

typedef enum {
    TAPE_NO_TAPE,
    TAPE_PHYSICAL_LOAD,
    TAPE_TAP_LOAD,
    TAPE_TZX_LOAD,
    TAPE_PHYSICAL_SAVE,
    TAPE_TAP_SAVE,
    TAPE_TZX_SAVE
} tapemode_t;

void tape__init();

bool tape__is_tape_loaded(void);
bool tape__is_tape_playing(void);
bool tape__is_tape_saving(void);

tapemode_t tape__get_tape_mode(void);
int tape__enter_tape_mode(tapemode_t);


void tape__notify_load(void);
void tape__notify_save(void);
void tape__notify_mic_idle(uint8_t value);


#endif
