#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <inttypes.h>

void keyboard__init(void);
void keyboard__press(uint8_t key);
void keyboard__release(uint8_t key);

#define SKEY(x,y) ((x*5)+y)

#define SPECT_KEYIDX_SHIFT SKEY(0,0)
#define SPECT_KEYIDX_Z     SKEY(0,1)
#define SPECT_KEYIDX_X     SKEY(0,2)
#define SPECT_KEYIDX_C     SKEY(0,3)
#define SPECT_KEYIDX_V     SKEY(0,4)

#define SPECT_KEYIDX_A     SKEY(1,0)
#define SPECT_KEYIDX_S     SKEY(1,1)
#define SPECT_KEYIDX_D     SKEY(1,2)
#define SPECT_KEYIDX_F     SKEY(1,3)
#define SPECT_KEYIDX_G     SKEY(1,4)

#define SPECT_KEYIDX_Q     SKEY(2,0)
#define SPECT_KEYIDX_W     SKEY(2,1)
#define SPECT_KEYIDX_E     SKEY(2,2)
#define SPECT_KEYIDX_R     SKEY(2,3)
#define SPECT_KEYIDX_T     SKEY(2,4)

#define SPECT_KEYIDX_1     SKEY(3,0)
#define SPECT_KEYIDX_2     SKEY(3,1)
#define SPECT_KEYIDX_3     SKEY(3,2)
#define SPECT_KEYIDX_4     SKEY(3,3)
#define SPECT_KEYIDX_5     SKEY(3,4)

#define SPECT_KEYIDX_0     SKEY(4,0)
#define SPECT_KEYIDX_9     SKEY(4,1)
#define SPECT_KEYIDX_8     SKEY(4,2)
#define SPECT_KEYIDX_7     SKEY(4,3)
#define SPECT_KEYIDX_6     SKEY(4,4)

#define SPECT_KEYIDX_P     SKEY(5,0)
#define SPECT_KEYIDX_O     SKEY(5,1)
#define SPECT_KEYIDX_I     SKEY(5,2)
#define SPECT_KEYIDX_U     SKEY(5,3)
#define SPECT_KEYIDX_Y     SKEY(5,4)

#define SPECT_KEYIDX_ENTER SKEY(6,0)
#define SPECT_KEYIDX_L     SKEY(6,1)
#define SPECT_KEYIDX_K     SKEY(6,2)
#define SPECT_KEYIDX_J     SKEY(6,3)
#define SPECT_KEYIDX_H     SKEY(6,4)

#define SPECT_KEYIDX_SPACE SKEY(7,0)
#define SPECT_KEYIDX_SYM   SKEY(7,1)
#define SPECT_KEYIDX_M     SKEY(7,2)
#define SPECT_KEYIDX_N     SKEY(7,3)
#define SPECT_KEYIDX_B     SKEY(7,4)

#endif
