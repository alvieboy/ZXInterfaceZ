#include "spectctrl.h"
#include "fpga.h"
#include "fasttap.h"

void spectctrl__reset(void)
{
    fasttap__stop();
    fpga__reset_spectrum();
}
