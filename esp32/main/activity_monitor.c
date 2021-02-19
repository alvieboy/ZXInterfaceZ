#include "activity_monitor.h"
#include "fpga.h"

int activity_monitor__read_mic_idle()
{
    return fpga__read_mic_idle();
}

