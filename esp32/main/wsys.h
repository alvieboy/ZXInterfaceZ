#ifndef __WSYS_H__
#define __WSYS_H__

/**
 * \defgroup wsys Windowing SYStem
 * \brief Windowing SYStem
 *
 * \ingroup wsys
 * \defgroup wsyswidget WSYS Widgets
 * \brief WSYS Widget subsystem
 *
 * Widgets are the base of all windowing system.
 *
*/


#include <inttypes.h>

#define EVENT_KBD 0
#define EVENT_NMIENTER 1
#define EVENT_RESET 2
#define EVENT_NMILEAVE 3
#define EVENT_MEMORYREADCOMPLETE 4
#define EVENT_JOYSTICK 5
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
void wsys__memoryreadcomplete(uint8_t size);
void wsys__requestmemread(uint16_t address, uint8_t size, void(*callback)(void*,uint8_t),void*user);
void wsys__requestromread(uint16_t address, uint8_t size, void(*callback)(void *,uint8_t),void*user);

#ifdef __cplusplus
}
#endif

// C++.
void wsys__eventloop_iter();

#endif
