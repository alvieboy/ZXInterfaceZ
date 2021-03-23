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

#define DEVMAP_JSON_FILENAME "/config/devmap.jsn"
#define TAG "DEVMAP"
#define MAX_INTERFACES 2

/**
 \defgroup devmap Device mapper
 \brief Device mapper

 The device mapper is responsible from dispatching events (such as HID events) from a
 particular device into actions

 */

/**
 \ingroup devmap
 \brief Mapping type for event
 */
enum map_type {
    MAP_NONE /** No mapping */,
    MAP_KEYBOARD /** Map to keyboard */,
    MAP_JOYSTICK /** Map to joystick */,
    MAP_NMI /** Map to NMI(USR) key */,
    MAP_MOUSE /** Map to mouse */
};

static struct {
    const char *name;
    enum map_type map;
} map_entries[] = {
    { "keyboard", MAP_KEYBOARD },
    { "joystick", MAP_JOYSTICK },
    { "nmi", MAP_NMI },
    { "mouse", MAP_MOUSE },
};


/**
 \ingroup devmap
 \brief Devmap entry linked list entry
 */
typedef struct devmap_e {
    struct devmap_e *next;
    enum map_type map:8;
    unsigned index:8;
    int16_t analog_threshold;
    unsigned action_value:16;
} devmap_e_t;

/**
 \ingroup devmap
 \brief Devmap device linked list entry
 */
typedef struct devmap_d {
    struct devmap_d *next;
    uint32_t id;
    char *manufacturer;
    char *product;
    char *serial;
    uint8_t interfaces[MAX_INTERFACES];
    devmap_e_t *entries[MAX_INTERFACES];
} devmap_d_t;

/**
 \ingroup devmap
 \brief Root devmap
 */
static devmap_d_t *root_devmap;

/**
 \ingroup devmap
 \brief Convert a map type to its name
 \param type The map type
 \return the map name, or "unknown" if type was not found.
 */
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

/**
 \ingroup devmap
 \brief Parse a map name into its value
 \param str The map name
 \return the map type, or MAP_NONE if type was not found.
 */
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

/**
 \ingroup devmap
 \brief Parse a textual usb ID (VVVV:PPPP) into a uint32_t value.

 The vendor part will be placed in the upper 16 bits, while the product part
 will be in the lower 32-bits.

 Example: converting the id "dead:beef" will return 0xDEADBEEF

 \param str The textual ID
 \param target_device Pointer to a 32-bit word where the ID will be stored
 \return 0 if successful, -1 if any parsing error occurred
 */
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

/**
 \ingroup devmap
 \brief Free all entries on a devmap entry linked list.
 \param d the entries linked list
 */
static void devmap__free_entries(devmap_e_t *d)
{
    if (!d)
        return;

    if (d->next)
        devmap__free_entries(d->next);
    else
        free(d);
}

/**
 \ingroup devmap
 \brief Free a devmap entry linked list.

 It will free also all entries on the devmap for all interfaces.

 \param d the devmap linked list
 */
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

/**
 \ingroup devmap
 \brief Parse a JSON file into a devmap entry lst
 \param filename The JSON filename
 \return A devmap entry linked list, or NULL if any error occurred while parsing
 */
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

/**
 \ingroup devmap
 \brief Initalise the devmap.

 It will be loaded from DEVMAP_JSON_FILENAME
 */
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


/**
 \ingroup devmap
 \brief Find devmap device to handle HID device

 Given a particular HID device, find if any devmap device can handle events from it.

 \param dev The HID device
 \return The devmap device, or NULL if not found
 */
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


/**
 \ingroup devmap
 \brief Get a digital boolean value from a HID field



 \param field The HID field
 \param uvalue The HID field value
 \param analog_threshold the analog threshold, if applicable
 \return True is the value represents true, false otherwise
 */
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


/**
 \ingroup devmap
 \brief Trigger action entry for a devmap entry

 \param entry The devmap entry
 \param field The corresponding HID field definition
 \param value The field value
 */

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

/**
 \ingroup devmap
 \brief HID callback for field changes

 This method is called from within the HID routines whenver a field changes value
 */
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
