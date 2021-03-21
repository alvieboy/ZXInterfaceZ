#include "hid.h"
#include <stddef.h>
#include "keyboard.h"
#include "esp_log.h"
#include "fpga.h"
#include <malloc.h>
#include "json.h"
#include <string.h>
#include "devmap.h"
#include "strtoint.h"
#include "joystick.h"
#include "log.h"

#define DEVMAPTAG "DEVMAP"
#define DEVMAPDEBUG(x...) LOG_DEBUG(DEBUG_ZONE_DEVMAP, DEVMAPTAG ,x)


enum map_type {
    MAP_NONE,
    MAP_KEYBOARD,
    MAP_JOYSTICK,
    MAP_NMI,
    MAP_MOUSE
};

typedef struct devmap_e {
    struct devmap_e *next;
    enum map_type map:8;
    unsigned index:8;
    int16_t analog_threshold;
    unsigned action_value:16;
} devmap_e_t;

#define MAX_INTERFACES 2

typedef struct devmap_d {
    struct devmap_d *next;
    uint32_t id;
    char *manufacturer;
    char *product;
    char *serial;
    uint8_t interfaces[MAX_INTERFACES];
    devmap_e_t *entries[MAX_INTERFACES];
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

static struct {
    const char *name;
    enum map_type map;
} map_entries[] = {
    { "keyboard", MAP_KEYBOARD },
    { "joystick", MAP_JOYSTICK },
    { "nmi", MAP_NMI },
    { "mouse", MAP_MOUSE },
};

            
static const char *devmap__map_name_from_type(enum map_type type)
{
    unsigned i;
    for (i=0;i<sizeof(map_entries)/sizeof(map_entries[0]);i++) {
        if (map_entries[i].map == type) {
            return map_entries[i].name;
        }
    }
    return "unknown";
}

static enum map_type devmap__parse_map(const char *str)
{
    unsigned int i;
    for (i=0;i<sizeof(map_entries)/sizeof(map_entries[0]);i++) {
        if (strcmp(map_entries[i].name, str)==0) {
            return map_entries[i].map;
        }
    }
    return MAP_NONE;
}

static int devmap__parse_usb_id(const char *str, uint32_t *target_device)
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
    hex[4] = '\0';

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

static void devmap__free_entries(devmap_e_t *d)
{
    if (!d)
        return;

    if (d->next)
        devmap__free_entries(d->next);
    else
        free(d);
}

static void devmap__free_d(devmap_d_t *d)
{
    if (d->serial)
        free(d->serial);
    if (d->manufacturer)
        free(d->manufacturer);
    if (d->product)
        free(d->product);
    if (d->entries[0])
        devmap__free_entries(d->entries[0]);
    if (d->entries[1])
        devmap__free_entries(d->entries[1]);
    free(d);
}

static devmap_e_t *devmap__parse_entries(const char *filename)
{
    devmap_e_t *e_root = NULL, *c;
    cJSON *root = json__load_from_file(filename);
    if (root) {

        cJSON *entries = cJSON_GetObjectItemCaseSensitive(root, "entries");
        cJSON *entry;

        cJSON_ArrayForEach(entry, entries) {
            c = malloc(sizeof(devmap_e_t));
            if (!c) {
                devmap__free_entries(e_root);
                return NULL;
            }
            // Append to list
            c->next = e_root;
            e_root = c;
            const char *mapstr = json__get_string(entry,"map");
            c->map = devmap__parse_map(mapstr);
            c->index = json__get_integer(entry, "index");
            c->analog_threshold = json__get_integer_default(entry, "threshold", 0);
            if (c->map == MAP_KEYBOARD) {
                const char *keybname = json__get_string(entry,"value");
                c->action_value = keyboard__get_key_by_name(keybname);
            } else if (c->map == MAP_JOYSTICK) {
                const char *joyname = json__get_string(entry,"value");
                c->action_value = joystick__get_action_by_name(joyname);
            } else {
                c->action_value = json__get_integer_default(entry, "value", 0);
            }
            DEVMAPDEBUG("New map entry %s idx=%d thresh=%d action=%d",
                     devmap__map_name_from_type(c->map),
                     c->index,
                     c->analog_threshold,
                     c->action_value);
        }
    } else {
        ESP_LOGE(TAG, "Could not parse '%s'", filename);
    }
    cJSON_Delete(root);
    return e_root;
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
        uint8_t interface_index = 0;

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

        DEVMAPDEBUG("Handling %s %s", dm->manufacturer, dm->product);

        // Load config file
        const char *configfile = json__get_string(device,"config");
        if (configfile) {
            // Parse it
            dm->entries[interface_index] = devmap__parse_entries(configfile);
            dm->interfaces[interface_index] = -1;
            if (!dm->entries) {
                ESP_LOGE(TAG,"Could not load entries for '%s' (%s)", id, configfile?configfile:"null");
            }
        } else {
            DEVMAPDEBUG("No config file specified");
        }
        // if we have multiple configs (for example, multi interface entities), load them

        cJSON *configs = cJSON_GetObjectItemCaseSensitive(device, "configs");
        cJSON *config;

        cJSON_ArrayForEach(config, configs) {

            const char *key = json__get_key(config);
            int key_int;

            if (strtoint(key, &key_int)<0) {
                ESP_LOGW(TAG,"Invalid JSON key '%s', skipping", key);
                continue;
            }

            dm->interfaces[interface_index] = key_int;
            configfile = NULL;

            if (cJSON_IsString(config)) {
                configfile = config->valuestring;
            }
            DEVMAPDEBUG("Loading interface %d", key_int);

            if (configfile) {
                // Parse it
                dm->entries[interface_index] = devmap__parse_entries(configfile);
                if (!dm->entries[interface_index]) {
                    ESP_LOGE(TAG,"Could not load entries for '%s' (%s)", id, configfile?configfile:"null");
                }
                interface_index++;
            } else {
                DEVMAPDEBUG("No config file (array) specified");
            }
        }

        DEVMAPDEBUG("Loaded %s", id);
        dm->next = root_devmap;
        root_devmap = dm;
    }

    cJSON_Delete(root);

}


#if 0
static int devmap__save_to_file(const char *filename, const devmap_e_t *devmap)
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
#endif

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
        devmap = devmap->next;
    }
    return NULL;
}


static bool devmap__get_digital(const struct hid_field *field, uint8_t uvalue, int16_t analog_threshold)
{
    int32_t value;

    // Check if value is signed
    if (field->logical_minimum < 0) {
        // Signed value. Perform sign extent.
        uint32_t raw = uvalue;
        if (uvalue & (1<<(field->report_size-1))) {
            // Signed. extend
            raw |= ~((1<<(field->report_size-1))-1);
        }
        value = (int32_t)raw;
    } else {
        value = uvalue;
    }

    DEVMAPDEBUG("Field phys_min %d phys_max %d logical_min %d logical_max %d value %d (raw %02x)",
                field->physical_minimum,
                field->physical_maximum,
                field->logical_minimum,
                field->logical_maximum,
                value, uvalue);

    if ((field->logical_minimum == 0) && (field->logical_maximum==1))
        return value!=0;
    // For analog, find center.

    int center = ((field->logical_maximum - field->logical_minimum)>>1)+field->logical_minimum;

    DEVMAPDEBUG("Center %d, analog threshold %d", center, analog_threshold);

    if (analog_threshold>0) {
        return (value >= (center+analog_threshold));
    } else {
        return (value <= (center+analog_threshold));
    }
}



static void devmap__trigger_entry(const devmap_e_t *entry, const struct hid_field* field, uint8_t value)
{
    bool on;

    on = devmap__get_digital(field, value, entry->analog_threshold);

    switch (entry->map) {
    case MAP_KEYBOARD:
        DEVMAPDEBUG("Performing action %s on key %d", on?"ON":"OFF", entry->action_value);
        if (on) {
            keyboard__press(entry->action_value);
        } else {
            keyboard__release(entry->action_value);
        }
        break;
    case MAP_JOYSTICK:
        DEVMAPDEBUG("Performing action %s on joystick %d", on?"ON":"OFF", entry->action_value);
        if (on) {
            joystick__press(entry->action_value);
        } else {
            joystick__release(entry->action_value);
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

    if (d==NULL) {
        ESP_LOGI(TAG, "Cannot find device to handle event");

        ESP_LOGI(TAG, " > Field changed start %d len %d index %d value=%d",
                 field->report_offset,
                 field->report_size,
                 entry_index,
                 new_value
                );

        return;
    }

    int ifn = hid__get_interface(dev);

    for (int interface_index=0; interface_index<2; interface_index++) {

        if (ifn!=d->interfaces[interface_index]) {
            continue;
        }

        for (map = d->entries[interface_index]; map!=NULL; map=map->next) {

            if (map->index  == entry_index) {
                devmap__trigger_entry(map, field, new_value);
            }
        }
    }
}

static bool devmap__is_connected(const devmap_d_t *d)
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
