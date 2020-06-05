#include "hid.h"

typedef struct {
    enum {
        MAP_KEYBOARD,
        MAP_JOYSTICK,
        MAP_MOUSE
    } map_type:8;
    unsigned index:8;
    unsigned value:16;
} devmap_e_t;

typedef struct {
    device_id id;
    uint8_t map_entries;
    devmap_e_t *entries;
} devmap_d_t;





void hid__field_entry_changed_callback(const device_id dev_id, const struct hid_field* field, uint8_t entry_index, uint8_t new_value)
{

}
