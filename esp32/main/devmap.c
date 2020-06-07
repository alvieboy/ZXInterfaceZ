#include "hid.h"
#include <stddef.h>
#include "keyboard.h"
#include "esp_log.h"
#include "fpga.h"

typedef struct {
    enum {
        MAP_KEYBOARD,
        MAP_JOYSTICK,
        MAP_NMI,
        MAP_MOUSE
    } map_type:8;
    unsigned index:8;
    uint16_t analog_threshold;
    unsigned action_value:16;
} devmap_e_t;

typedef struct {
    device_id id;
    uint8_t num_entries;
    const devmap_e_t *entries;
} devmap_d_t;

/* For testing purposes */
/*
 Gamepad indexes
 6: key 1
 7: key 2
 8: key 3
 9: key 3
 7: rotR horiz
 6: rotR vert
 2: roTl horiz
 3: roTl horiz

 */

/*I (22571) HID: Report 0 size 56
I (22571) HID:  > Field offset 0 size 8 count 5
I (22581) HID:   [0] usage 0x0132 (Z axis)
I (22581) HID:   [1] usage 0x0135 (Z rotation)
I (22591) HID:   [2] usage 0x0130 (X axis)
I (22591) HID:   [3] usage 0x0131 (Y axis)
I (22601) HID:   [4] usage 0x0100 (Undefined)
I (22601) HID:  > Field offset 40 size 4 count 1
I (22611) HID:   [0] usage 0x0139 (Hat switch)
I (22611) HID:  > Field offset 44 size 1 count 12
I (22621) HID:   [0] usage 0x0901 (Button 1)
I (22621) HID:   [1] usage 0x0902 (Button 2)
I (22631) HID:   [2] usage 0x0903 (Button 3)
I (22631) HID:   [3] usage 0x0904 (Button 4)
I (22641) HID:   [4] usage 0x0905 (Button 5)
I (22641) HID:   [5] usage 0x0906 (Button 6)
I (22651) HID:   [6] usage 0x0907 (Button 7)
I (22651) HID:   [7] usage 0x0908 (Button 8)
I (22661) HID:   [8] usage 0x0909 (Button 9)
I (22661) HID:   [9] usage 0x090a (Button 10)
I (22671) HID:   [10] usage 0x090b (Button 11)
I (22671) HID:   [11] usage 0x090c (Button 12)
          */
static const devmap_e_t gamepad_devmap[] = {
    { MAP_KEYBOARD, 6, 0, SPECT_KEYIDX_Q },
    { MAP_KEYBOARD, 7, 0, SPECT_KEYIDX_A },
    { MAP_KEYBOARD, 4, 0, SPECT_KEYIDX_O },
    { MAP_KEYBOARD, 5, 0, SPECT_KEYIDX_P },
    { MAP_KEYBOARD, 8, 0,  SPECT_KEYIDX_M },
    { MAP_KEYBOARD, 2, +64,  SPECT_KEYIDX_P }, // Analog right
    { MAP_KEYBOARD, 2, -64,  SPECT_KEYIDX_O }, // Analog left
    { MAP_KEYBOARD, 3, +64,  SPECT_KEYIDX_Q }, // Analog up
    { MAP_KEYBOARD, 3, -64,  SPECT_KEYIDX_A }, // Analog down
};

static const devmap_d_t devmap[] = {
    { 0x0e8f0003 , sizeof(gamepad_devmap)/sizeof(gamepad_devmap[0]), gamepad_devmap }
};



static const devmap_d_t *devmap__find_by_id(const device_id dev_id)
{
    unsigned i;
    for (i=0;i<sizeof(devmap)/sizeof(devmap[0]);i++) {
        if (devmap[i].id == dev_id) {
            return &devmap[i];
        }
    }
    return NULL;
}


static bool devmap__get_digital(const struct hid_field *field, uint8_t value, int16_t analog_threshold)
{
    ESP_LOGI("devmap", "Field phys_min %d phys_max %d logical_min %d logical_max %d value %d",
             field->physical_minimum,
             field->physical_maximum,
             field->logical_minimum,
             field->logical_maximum,
             value);
    if ((field->logical_minimum == 0) && (field->logical_maximum==1))
        return value!=0;
    // For analog, find center.
    int center = (field->logical_maximum - field->logical_minimum)>>1;
    ESP_LOGI("devmap", "Center %d, analog threshold %d", center, analog_threshold);

    if (analog_threshold>0) {
        return (value > (center+analog_threshold));
    } else {
        return (value < (center+analog_threshold));
    }
}



static void devmap__trigger_entry(const devmap_e_t *entry, const struct hid_field* field, uint8_t value)
{
    bool on;

    on = devmap__get_digital(field, value, entry->analog_threshold);

    switch (entry->map_type) {
    case MAP_KEYBOARD:
        ESP_LOGI("devmap", "Performing action %s on key %d", on?"ON":"OFF", entry->action_value);
        if (on) {
            keyboard__press(entry->action_value);
        } else {
            keyboard__release(entry->action_value);
        }
        break;
    case MAP_NMI:
        if (on) {
            fpga__set_trigger(FPGA_FLAG_TRIG_FORCENMI_ON);
        }
        break;
    default:
        break;
    }
}

void hid__field_entry_changed_callback(const device_id dev_id, const struct hid_field* field, uint8_t entry_index, uint8_t new_value)
{
    const devmap_d_t *d = devmap__find_by_id(dev_id);

    if (d==NULL)
        return;

   // (void)devmap__get_digital(field, new_value, entry->analog_threshold);

    unsigned i;
    for (i=0;i<d->num_entries;i++) {

        const devmap_e_t *map = &d->entries[i];

        if (map->index  == entry_index) {
            devmap__trigger_entry(map, field, new_value);
        }
    }
//    ESP_LOGW("DEVMAP", "Cannot find handle for index %d", entry_index);
}
