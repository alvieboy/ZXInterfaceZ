#include "save.h"
#include "fpga.h"

void save__notify_save_to_tap(const char *path, const char *filename)
{
    fpga__write_miscctrl(0x01);
}

void save__notify_no_save()
{
    fpga__write_miscctrl(0xFF);
}
