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
 3: roTl vert
 10: Left 2
 11: Right 2
 12: Left 1
 13: Right 1
 14: select
 15: Start


 */
static const devmap_e_t gamepad_devmap[] = {
    { MAP_NMI,     14, 0, 0 }, // Select button
    { MAP_KEYBOARD,15, 0, SPECT_KEYIDX_ENTER }, // Start button

    { MAP_KEYBOARD, 6, 0, SPECT_KEYIDX_Q },
    { MAP_KEYBOARD, 7, 0, SPECT_KEYIDX_A },

    { MAP_KEYBOARD, 4, 0, SPECT_KEYIDX_O },
    { MAP_KEYBOARD,12, 0, SPECT_KEYIDX_O }, // Left button 1
    { MAP_KEYBOARD, 5, 0, SPECT_KEYIDX_P },
    { MAP_KEYBOARD,13, 0, SPECT_KEYIDX_P }, // Right button 1

    { MAP_KEYBOARD, 8, 0,  SPECT_KEYIDX_M },

    { MAP_KEYBOARD, 2, +64,  SPECT_KEYIDX_P }, // Analog right
    { MAP_KEYBOARD, 2, -64,  SPECT_KEYIDX_O }, // Analog left
    { MAP_KEYBOARD, 3, +64,  SPECT_KEYIDX_A }, // Analog down
    { MAP_KEYBOARD, 3, -64,  SPECT_KEYIDX_Q }, // Analog up
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
