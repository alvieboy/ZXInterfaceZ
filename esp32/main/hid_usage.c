#include <inttypes.h>
#include "hid_usage.h"

static const uint8_t page0_idx_table[] = {
    0x00,//  Undefined
    0x01, // Pointer
    0x02,  // Mouse
    0x04, // Joystick CA 4.1
    0x05, // Game Pad CA 4.1
    0x06, // Keyboard CA 4.1
    0x30, // X DV 4.2
    0x31, // Y DV 4.2
    0x32, // Z DV 4.2
    0x33, // Rx DV 4.2
    0x34, // Ry DV 4.2
    0x35, // Rz DV 4.2
    0x36, // Slider DV 4.3
    0x37, // Dial DV 4.3
    0x38, // Wheel DV 4.3
    0x39, // Hat switch DV 4.3
    0x3D, // Start OOC 4.3
    0x3E, // Select OOC 4.3
    0x90, // D-pad Up OOC 4.7
    0x91, // D-pad Down OOC 4.7
    0x92, // D-pad Right OOC 4.7
    0x93, // D-pad Left OOC 4.7
};

static const char *page0_str_table[] = {
    "Undefined",
    "Pointer",
    "Mouse",
    "Joystick",
    "Game Pad",
    "Keyboard",
    "X axis",
    "Y axis",
    "Z axis",
    "X rotation",
    "Y rotation",
    "Z rotation",
    "Slider",
    "Dial",
    "Wheel",
    "Hat switch",
    "Start",
    "Select",
    "D-pad Up",
    "D-pad Down",
    "D-pad Right",
    "D-pad Left"
};

const char *buttons_table[] = {
    "No button",
    "Button 1",
    "Button 2",
    "Button 3",
    "Button 4",
    "Button 5",
    "Button 6",
    "Button 7",
    "Button 8",
    "Button 9",
    "Button 10",
    "Button 11",
    "Button 12",
    "Button 13",
    "Button 14",
    "Button 15",
    "Button 16",
    "Button",
};


static const char *hid_usage_table_find(const uint8_t *idxtable, unsigned idxtablesize, const char **strtable, uint16_t usage)
{
    unsigned i;
    for (i=0;i<idxtablesize;i++) {
        if (idxtable[i] == usage)
            return strtable[i];
    }
    return strtable[0];
}

const char *hid_usage_find(uint16_t usage)
{
    uint8_t page = usage>>8;
    usage &= 0xff;
    if (page==0x01) {
        return hid_usage_table_find(page0_idx_table, sizeof(page0_idx_table), page0_str_table, usage);
    } else if (page==0x9) {
        if (usage>17) {
            usage=17;
        }
        return buttons_table[usage];
    }
    return "Unknown";
}
