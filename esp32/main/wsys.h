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




void wsys__keyboard_event(uint16_t raw, char ascii);
void wsys__nmiready();
void wsys__nmileave();
void wsys__reset();
void wsys__init();
void wsys__send_to_fpga();
void wsys__get_screen_from_fpga();
void wsys__send_command(uint8_t command);

#ifdef __cplusplus
}
#endif

void wsys__eventloop_iter();


