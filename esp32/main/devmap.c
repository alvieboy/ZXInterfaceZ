#include "hid.h"
#include <stddef.h>
#include "keyboard.h"
#include "esp_log.h"
#include "fpga.h"
#include <malloc.h>
#include "json.h"
#include <string.h>
#include "devmap.h"

enum map_type {
        MAP_KEYBOARD,
        MAP_JOYSTICK,
        MAP_NMI,
        MAP_MOUSE
};

typedef struct devmap_e {
    struct devmap_e *next;
    enum map_type map:8;
    unsigned index:8;
    uint16_t analog_threshold;
    unsigned action_value:16;
} devmap_e_t;

typedef struct devmap_d {
    struct devmap_d *next;
    uint32_t id;
    char *manufacturer;
    char *product;
    char *serial;
    devmap_e_t *entries;
} devmap_d_t;

static devmap_d_t *root_devmap;


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
/*static const devmap_e_t gamepad_devmap[] = {
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
*/



#define DEVMAP_JSON_FILENAME "/config/devmap.jsn"
#define TAG "Devmap"

int devmap__parse_usb_id(const char *str, uint32_t *target_device)
{
    char hex[5];
    char *end = NULL;

    if (strlen(str)!=9) {
        ESP_LOGE(TAG,"ID too short");
        return -1;
    }

    if (str[4]!=':') {
        ESP_LOGE(TAG,"ID missing separator");
        return -1;
    }

    strncpy(hex, str, 4);

    unsigned long vendor = strtoul(hex, &end,16);
    if (end==NULL || *end!='\0') {
        ESP_LOGE(TAG,"Cannot parse vendor '%s'", hex);
        return -1;
    }
    end = NULL;
    unsigned long product = strtoul(&str[5], &end,16);

    if (end==NULL || *end!='\0') {
        ESP_LOGE(TAG,"Cannot parse product '%s'", &str[5]);
        return -1;
    }
    *target_device = ((uint32_t)vendor << 16)+ (uint32_t)product;
    return 0;
}

void devmap__free_d(devmap_d_t *d)
{
    if (d->serial)
        free(d->serial);
    if (d->manufacturer)
        free(d->manufacturer);
    if (d->product)
        free(d->product);
    free(d);
}


void devmap__init()
{
    cJSON *root = json__load_from_file(DEVMAP_JSON_FILENAME);

    if (!root) {
        ESP_LOGW(TAG,"Could not load JSON devmap");
        return;
    }

    cJSON *devices = cJSON_GetObjectItemCaseSensitive(root, "devices");
    cJSON *device;

    cJSON_ArrayForEach(device, devices) {
        devmap_d_t *dm = calloc(1,sizeof(devmap_d_t));

        const char *id = json__get_string(device,"id");
        if (id==NULL) {
            ESP_LOGE(TAG,"Cannot find ID node");
            devmap__free_d(dm);
            break;
        }
        if (devmap__parse_usb_id(id, &dm->id)!=0) {
            ESP_LOGE(TAG,"Error parsing ID '%s'", id);
            devmap__free_d(dm);
            break;
        }

        dm->serial = json__get_string_alloc(device,"serial");
        dm->manufacturer = json__get_string_alloc(device,"manufacturer");
        dm->product = json__get_string_alloc(device,"product");

        ESP_LOGI(TAG,"Loaded %s", id);
        dm->next = root_devmap;
        root_devmap = dm;
    }

    cJSON_Delete(root);

}

            
const char *devmap__map_name_from_type(enum map_type type)
{
    switch (type) {
    case MAP_KEYBOARD: return "keyboard";
    case MAP_JOYSTICK: return "joystick";
    case MAP_NMI:      return "nmi";
    case MAP_MOUSE:    return "mouse";
    default:           return "unknown";
    }
}

int devmap__save_to_file(const char *filename, const devmap_d_t *devmap)
{
    char did[10];

    cJSON *root = cJSON_CreateObject();

    while (devmap!=NULL) {

        cJSON *entry = cJSON_CreateObject(); // device entry
        cJSON *mapping = cJSON_CreateArray(); // device map entries

        cJSON_AddItemToObject(entry, "default", mapping);

        // Populate entries
        devmap_e_t *e;

        for (e=devmap->entries; e!=NULL; e=e->next) {
            cJSON *item = cJSON_CreateObject();

            cJSON_AddNumberToObject(item, "index", e->index);
            cJSON_AddStringToObject(item, "map", devmap__map_name_from_type(e->map));
            if (e->analog_threshold!=0)
                cJSON_AddNumberToObject(item, "threshold", e->analog_threshold);
            switch (e->map) {
            case MAP_KEYBOARD:
                cJSON_AddStringToObject(item, "value", keyboard__get_name_by_key(e->action_value));
                break;
            default:
                break;
            }
            cJSON_AddItemToArray(mapping, item);
        }

        sprintf(did,"%04x:%04x",
                (devmap->id >> 16),
                (devmap->id & 0xffff));


        cJSON_AddItemToObject(root, did, entry);

        devmap = devmap->next;
    }


    char *data = cJSON_Print(root);


    free(data);
    cJSON_Delete(root);

    return 0;
}

static const devmap_d_t *devmap__find(const hid_device_t *dev)
{
    devmap_d_t *devmap = root_devmap;
    
    while (devmap!=NULL) {
        switch (dev->bus) {
        case HID_BUS_USB:

            if (devmap->id == hid__get_id(dev)) {

                return devmap;
            }
        default:
            break;
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

    switch (entry->map) {
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

void hid__field_entry_changed_callback(const hid_device_t *dev, const struct hid_field* field, uint8_t entry_index, uint8_t new_value)
{
    const devmap_d_t *d = devmap__find(dev);

    const devmap_e_t *map;

    if (d==NULL)
        return;

   // (void)devmap__get_digital(field, new_value, entry->analog_threshold);

//    unsigned i;

    for (map = d->entries; map!=NULL; map=map->next) {

        if (map->index  == entry_index) {
            devmap__trigger_entry(map, field, new_value);
        }
    }
//    ESP_LOGW("DEVMAP", "Cannot find handle for index %d", entry_index);
}

bool devmap__is_connected(const devmap_d_t *d)
{
    hid_device_t devs[DEVMAP_MAX_DEVICES];
    unsigned num_devices;
    unsigned i;

    hid__get_devices(&devs[0], &num_devices, DEVMAP_MAX_DEVICES);

    for (i=0;i<num_devices;i++) {
        //device_id id = usbh__get_device_id(e->device);
    }
    return false;
}



void devmap__populate_devices(cJSON *node)
{
    char idstr[10];
    cJSON *devices = cJSON_CreateArray(); // device map entries

    cJSON_AddItemToObject(node, "devices", devices);

    const devmap_d_t *d = root_devmap;
    while (d) {
        cJSON *entry = cJSON_CreateObject();
        sprintf(idstr,"%04x:%04x", (d->id>>16), d->id&0xFFFF);
        cJSON_AddStringToObject(entry, "id", idstr);
        cJSON_AddStringToObject(entry, "manufacturer", d->manufacturer);
        cJSON_AddStringToObject(entry, "product", d->product);
        cJSON_AddStringToObject(entry, "serial", d->serial);
        cJSON_AddStringToObject(entry, "connected", devmap__is_connected(d) ? "true":"false");
        cJSON_AddNullToObject(entry, "use");
        cJSON_AddItemToArray(devices, entry);
        d = d->next;
    }
}
