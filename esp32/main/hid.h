#ifndef __HID_H__
#define __HID_H__

#include <inttypes.h>
#include <stdbool.h>

typedef uint32_t device_id_t;

#define MAX_HID_ENTRIES 32
#define MAX_HID_USAGES 32
#define MAX_HID_FIELDS 32

/*typedef struct {
    uint16_t offset_bit;
    uint8_t len_bit;
    uint8_t type;
    const char *name;
    uint8_t collection;
} hid_report_entry_t;
*/
typedef struct
{
    int8_t parent;
    uint8_t type;
} hid_collection_t;

struct hid_global_data {
    int16_t logical_minimum;
    int16_t logical_maximum;
    int16_t physical_minimum;
    int16_t physical_maximum;
    // exponent not used.
    //uint8_t exponent;
    uint8_t unit;
    uint8_t report_size;
    uint8_t report_count;
    uint8_t report_id;
};

typedef struct {
    uint8_t collection;
    uint8_t usage_page;
    uint8_t usage_min;
    uint8_t usage_max;
} hid_usage_t;

struct hid_local_data {
    //uint8_t usage[MAX_HID_USAGES];
    //uint8_t collection[MAX_HID_USAGES];
    hid_usage_t usages[MAX_HID_USAGES];
    uint8_t usage_index;
    uint8_t usage_minimum;
    //uint8_t usage_maximum;
};

typedef struct hid_field {
    struct hid_field *next;
    uint8_t     unit;
    uint8_t     flags;
    // Here we alias usages[0] to usage_min and usages[1] to usage_max for non-variable (i.e., array) data.
    //    uint16_t    usages[MAX_HID_USAGES];
    hid_usage_t usages[MAX_HID_USAGES];
    uint16_t 	report_offset;
    uint16_t 	report_size;
    uint16_t 	report_count;
    uint16_t 	report_type;
    int16_t	logical_minimum;
    int16_t 	logical_maximum;
    int16_t 	physical_minimum;
    int16_t 	physical_maximum;
    uint8_t usage_index;
} hid_field_t;

typedef struct hid_report {
    struct hid_report *next;
    hid_field_t *fields;
    uint16_t size;
    uint8_t id;
} hid_report_t;

#define MAX_COLLECTIONS 8

struct hid
{
    uint32_t id; // Will be device/product Id
    int8_t usage_page;
    int8_t usage;
    uint8_t num_entries;
    int8_t current_collection;
    uint8_t num_collections;
    hid_collection_t collections[MAX_COLLECTIONS];
    struct hid_global_data global;
    struct hid_local_data local;
    hid_report_t *reports;
};

#define HID_BUS_USB 0

typedef union {
    uint8_t bus;
} hid_device_t;

struct hid * hid__parse(const uint8_t *hid, int len);
void hid__free(struct hid*);
int hid__extract_field_aligned(const hid_field_t *field, uint8_t index, const uint8_t *data, uint8_t *dest);
void hid__field_entry_changed_callback(const hid_device_t *hiddev, const struct hid_field* field, uint8_t entry_index, uint8_t new_value);

uint32_t hid__get_id(const hid_device_t *hiddev);
const char *hid__get_serial(const hid_device_t *hiddev);
int hid__get_interface(const hid_device_t *hiddev);

void hid__get_devices(hid_device_t *devices, unsigned *num, unsigned max);
const char* hid__get_driver_name(const hid_device_t*hiddev);
int hid__field_get_usage(hid_field_t *field, int reportindex, uint16_t *usage_min, uint16_t *usage_max);
bool hid__has_multiple_reports(struct hid *h);
int hid__number_of_reports(struct hid *h);
hid_report_t *hid__find_report_by_id(struct hid *, uint8_t report_id, uint8_t *report_index_out);

#endif
