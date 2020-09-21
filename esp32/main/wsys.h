#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

void wsys__keyboard_event(uint16_t raw, char ascii);
void wsys__nmiready();
void wsys__reset();
void wsys__init();
void wsys__send_to_fpga();
void wsys__get_screen_from_fpga();
void wsys__send_command(uint8_t command);



#ifdef __cplusplus
}
#endif


