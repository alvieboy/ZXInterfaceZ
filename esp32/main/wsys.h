#ifndef __WSYS_H__
#define __WSYS_H__

#include <inttypes.h>

#define EVENT_KBD 0
#define EVENT_NMIENTER 1
#define EVENT_RESET 2
#define EVENT_NMILEAVE 3

#define EVENT_SYSTEM 16

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    WSYS_MODE_NMI,
    WSYS_MODE_LOAD,
    WSYS_MODE_SAVE
} wsys_mode_t;

void wsys__keyboard_event(uint16_t raw, char ascii);
void wsys__nmiready(void);
void wsys__nmileave(void);
void wsys__reset(wsys_mode_t mode);
void wsys__init(void);
void wsys__send_to_fpga(void);
void wsys__get_screen_from_fpga(void);
void wsys__send_command(uint8_t command);

#ifdef __cplusplus
}
#endif

// C++.
void wsys__eventloop_iter();

#endif
