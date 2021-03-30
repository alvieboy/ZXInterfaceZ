#include "inttypes.h"
#include <string.h>
#include "hid_usage.h"
#include "hid.h"

#ifdef __linux__
#include <stdio.h>
#include <malloc.h>
#define HIDDEBUG(x...) do {printf(x);puts("");  } while(0)
#define HIDERROR(x...) do {printf(x);puts(""); } while(0)
#else
#include "log.h"
#include <esp_log.h>
#define HIDDEBUG(x...) LOG_DEBUG(DEBUG_ZONE_HID, "HID", x)
#define HIDERROR(x...) ESP_LOGE("HID", x)
#endif

#define zalloc(s) calloc(1,(s))

static hid_report_t *hid_register_report(struct hid *hid, uint8_t id)
{
    hid_report_t *r = hid->reports;

    while (r!=NULL) {
        if (r->id == id)
            return r;
        r = r->next;
    }

    r = zalloc(sizeof(hid_report_t));

    if (r==NULL)
        return NULL;
    r->next = NULL;
    r->id = id;

    if (hid->reports==NULL) {
        hid->reports = r;
    } else {
        hid_report_t *p = hid->reports;

        while (p->next) {
            p = p->next;
        }
        p->next = r;
    }

    return r;
}

static void hid_destroy_reports(struct hid *hid)
{
}


#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE 0x01
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM 0x05
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM 0x09
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM 0x0D
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM 0x11
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT 0x15
#define HID_GLOBAL_ITEM_TAG_UNIT 0x19
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE 0x1D
#define HID_GLOBAL_ITEM_TAG_REPORT_ID 0x21
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT 0x25
#define HID_GLOBAL_ITEM_TAG_PUSH 0x29
#define HID_GLOBAL_ITEM_TAG_POP 0x2D

#define HID_USAGE_GENERIC_DESKTOP 0x01
#define HID_USAGE_GAME_CONTROLS 0x05

#define HID_GENERIC_DESKTOP_JOYSTICK 0x04
#define HID_GENERIC_DESKTOP_GAMEPAD 0x05

#define HID_MAIN_TAG_INPUT 0x20
#define HID_MAIN_TAG_OUTPUT 0x24
#define HID_MAIN_TAG_FEATURE 0x2C
#define HID_MAIN_TAG_FEATURE 0x2C
#define HID_MAIN_TAG_COLLECTION 0x28
#define HID_MAIN_TAG_END_COLLECTION 0x30

#define HID_LOCAL_TAG_USAGE         0x02
#define HID_LOCAL_TAG_USAGE_MINIMUM 0x06
#define HID_LOCAL_TAG_USAGE_MAXIMUM 0x0A
#define HID_LOCAL_TAG_DESIGNATOR_INDEX 0x0E
#define HID_LOCAL_TAG_DESIGNATOR_MINUMUM 0x12
#define HID_LOCAL_TAG_DESIGNATOR_MAXIMUM 0x16
#define HID_LOCAL_TAG_STRING_INDEX 0x1E
#define HID_LOCAL_TAG_STRING_MINIMUM 0x22
#define HID_LOCAL_TAG_STRING_MAXIMUM 0x26
#define HID_LOCAL_TAG_DELIMITER  0x2A

/* These assume little-endian systems */
static int hid_get_signed_data(const uint8_t *data, uint8_t len)
{
    int val = 0;
    switch (len) {
    case 1:
        val = (int)((const int8_t*)data)[0];
        break;
    case 2:
        val = (int)((const int16_t*)data)[0];
        break;
    case 3:
        val = (int)((const int32_t*)data)[0];
        break;
    }
    return val;
}

static unsigned int hid_get_unsigned_data(const uint8_t *data, uint8_t len)
{
    unsigned int val = 0;
    switch (len) {
    case 1:
        val = (unsigned int)((const uint8_t*)data)[0];
        break;
    case 2:
        val = (unsigned int)((const uint16_t*)data)[0];
        break;
    case 3:
        val = (unsigned int)((const uint32_t*)data)[0];
        break;
    }
    return val;

}

static void hid_report_append_field(hid_report_t *report, hid_field_t *field)
{
    field->next = NULL;
    if (report->fields == NULL) {
        report->fields = field;
    } else {
        hid_field_t *p = report->fields;
        while (p->next) {
            p=p->next;
        }
        p->next = field;
    }
}

static int hid_count_local_usages(struct hid_local_data *local)
{
    int cnt = 0;
    int i;
    for (i=0;i<local->usage_index;i++) {
        int usage_count = (local->usages[i].usage_max -
            local->usages[i].usage_min)+1;
        cnt+=usage_count;
    }
    return cnt;
}

static int hid_add_local_usage(struct hid_local_data *local,
                               uint8_t collection,
                               uint8_t page,
                               uint8_t usage_min,
                               uint8_t usage_max)
{
    int index = local->usage_index;

    if (index==MAX_HID_USAGES) {
        HIDERROR("Too many usages");
        return -1;
    }

    local->usages[index].collection = collection;
    local->usages[index].usage_page = page;
    local->usages[index].usage_min = usage_min;
    local->usages[index].usage_max = usage_max;

    HIDDEBUG("*** Add local usage idx=%d page 0x%02x min 0x%02x max 0x%02x",
             local->usage_index,
             page,
             usage_min,
             usage_max);

    local->usage_index++;
    return 0;
}

#define HID_DATA_FLAG_CONSTANT (1<<0)
#define HID_DATA_FLAG_VARIABLE (1<<1)
#define HID_DATA_FLAG_RELATIVE (1<<2)

static int hid_add_input(struct hid *h, uint8_t flags)
{
    unsigned offset;

    hid_report_t *report = hid_register_report(h, h->global.report_id);

    if (!report) {
        HIDERROR("Cannot register report");
        return -1;
    }

    int usage_entries = hid_count_local_usages(&h->local);

    offset = report->size;
    report->size += h->global.report_size * h->global.report_count;

    if (usage_entries<=0) {
        /* Ignore if no usage is registered */
        HIDDEBUG("Ignoring report, no usage registered");
        return 0;
    }


    HIDDEBUG("Usages: %d", usage_entries);
    HIDDEBUG("Report count: %d", h->global.report_count);

    // In case not all usages were registered.



#if 0
    if ((h->global.report_count) > usages) {
        usages = h->global.report_count;
    }
#endif

    hid_field_t *field = zalloc(sizeof(hid_field_t));

    if (!field)
        return 0;

    hid_report_append_field(report, field);

#if 0
    unsigned i;

    for (i = 0; i < usages; i++) {
        unsigned j = i;
        /* Duplicate the last usage we parsed if we have excess values */
        if (i >= h->local.usage_index)
            j = h->local.usage_index - 1;

        field->usages[i] = (h->local.usage[j]) | (h->usage_page << 8);
    }
#endif
    memcpy( field->usages, h->local.usages, sizeof(h->local.usages) );
    field->usage_index = h->local.usage_index;


    field->flags = flags;
    field->report_offset = offset;
    //field->report_type = report_type;
    field->report_size = h->global.report_size;
    field->report_count = h->global.report_count;
    field->logical_minimum = h->global.logical_minimum;
    field->logical_maximum = h->global.logical_maximum;
    field->physical_minimum = h->global.physical_minimum;
    field->physical_maximum = h->global.physical_maximum;
    //field->unit_exponent = h->global.unit_exponent;
    field->unit = h->global.unit;

    return 0;
}

static int parse_hid_main_entry(struct hid *h, uint8_t tag, const uint8_t *data, uint8_t len)
{
    int r = 0;

    HIDDEBUG("Main Tag %d len %d", tag, len);

    if (h->usage_page>0) {
        switch (tag) {
        case HID_MAIN_TAG_COLLECTION:
            if (h->num_collections>=MAX_COLLECTIONS) {
                HIDERROR("Too many collections");
                return -1;
            }
            uint8_t parent = h->current_collection;
            h->current_collection = h->num_collections;
            h->num_collections++;
            h->collections[h->current_collection].parent = parent;
            h->collections[h->current_collection].type = *data;

            // Clear global data
            memset(&h->global, 0, sizeof(h->global));
            memset(&h->local, 0, sizeof(h->local));

            //h->usage_page = 0;
            break;
        case HID_MAIN_TAG_END_COLLECTION:
            if (h->current_collection==-1) {
                HIDERROR("Collection end without collection");
                return -1;
            }
            h->current_collection = h->collections[h->current_collection].parent;
            break;
        case HID_MAIN_TAG_INPUT:
            HIDDEBUG("INPUT ");
            hid_add_input(h, *data);
            memset(&h->local, 0, sizeof(h->local));
            break;
        case HID_MAIN_TAG_OUTPUT: /* Fall-through */
        case HID_MAIN_TAG_FEATURE:
            memset(&h->local, 0, sizeof(h->local));
            // Ignore
            break;
        default:
            h->local.usage_index = 0;
            HIDERROR("Unsupported main tag 0x%02x", tag);
            return -1;
        }
    }
    return r;
}

static int parse_hid_local_entry(struct hid *h, uint8_t tag, const uint8_t *data, uint8_t len)
{
    int r = 0;
    unsigned usage_maximum;

    HIDDEBUG("Local Tag %d len %d", tag, len);

    switch (tag) {
    case HID_LOCAL_TAG_USAGE:

        HIDDEBUG("USAGE: 0x%02x (index %d)", *data, h->local.usage_index);

        unsigned usage =  hid_get_unsigned_data(data,len);
        if (hid_add_local_usage(&h->local,
                                h->current_collection,
                                h->usage_page,
                                usage,
                                usage)<0)
        {
            HIDERROR("Too many usages");
            return -1;
        }

        break;

    case HID_LOCAL_TAG_USAGE_MINIMUM:
        h->local.usage_minimum = hid_get_unsigned_data(data,len);
        break;
    case HID_LOCAL_TAG_USAGE_MAXIMUM:

        usage_maximum = hid_get_unsigned_data(data,len);

        // Fill usages
        //int i;
        int count = usage_maximum - h->local.usage_minimum;

        if (count<0) {
            HIDERROR("Negative usage count!!");
            return -1;
        }

        if (hid_add_local_usage(&h->local,
                                h->current_collection,
                                h->usage_page,
                                h->local.usage_minimum,
                                usage_maximum)<0)
        {
            HIDERROR("Too many usages");
            return -1;
        }

        break;

    default:
        HIDERROR("Unsupported local tag %02x\n", tag);
        r=-1;
        break;
    }
    return r;
}


static int parse_hid_global_entry(struct hid *h, uint8_t tag, const uint8_t *data, uint8_t len)
{
    int r = 0;

    HIDDEBUG("Global Tag %d len %d", tag, len);

    switch (tag) {
    case HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
        HIDDEBUG(" >> new usage page %d", *data);
        h->usage_page = *data;
        break;

    case HID_GLOBAL_ITEM_TAG_PUSH: /* Fall-through */
    case HID_GLOBAL_ITEM_TAG_POP:
        r=-1;
        break;
    case HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM:
        h->global.logical_minimum = hid_get_signed_data(data,len);
        break;

    case HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
        h->global.logical_maximum = hid_get_signed_data(data,len);
        break;

    case HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM:
        h->global.physical_minimum = hid_get_signed_data(data,len);
        break;

    case HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM:
        h->global.physical_maximum = hid_get_signed_data(data,len);
        break;

    case HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT:
        break;

    case HID_GLOBAL_ITEM_TAG_UNIT:
        // Do we need this???
        h->global.unit = hid_get_unsigned_data(data,len);
        break;
    case HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
        h->global.report_size = hid_get_unsigned_data(data,len);
        break;

    case HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
        //printf(">>>>> %d %02x %02x\n",len,data[0],data[1]);
        h->global.report_count = hid_get_unsigned_data(data,len);
        break;

    case HID_GLOBAL_ITEM_TAG_REPORT_ID:
        h->global.report_id = hid_get_unsigned_data(data,len);
        break;
    default:
        HIDERROR("Unexpected global tag 0x%02x", tag);
        r = -1;
        break;
    }
    return r;
}



int hid__field_get_usage(hid_field_t *field, int reportindex, uint16_t *usage_min, uint16_t *usage_max)
{
    //uint8_t usage_val;
    hid_usage_t usage;
    int usage_index=0;
    int offset = 0;

    if (field->flags & HID_DATA_FLAG_VARIABLE) {
        do {
            usage = field->usages[usage_index];
            if (reportindex==0) {
                *usage_min = (((uint16_t)usage.usage_page)<<8) | ((usage.usage_min + offset) & 0xff);
                *usage_max = (((uint16_t)usage.usage_page)<<8) | ((usage.usage_min + offset) & 0xff);
                return 0;
            }
            reportindex--;
            offset++;
            if (usage.usage_min+offset > usage.usage_max) {
                offset = 0;
                usage_index++;
            }
        } while (usage_index <= field->usage_index);
    } else {
        if (field->usage_index!=1) {
            HIDERROR("Unsupported array without usage range (%d)!", field->usage_index);
            return -1;
        }
        usage = field->usages[usage_index];
        *usage_min = (((uint16_t)usage.usage_page)<<8) | ((usage.usage_min) & 0xff);
        *usage_max = (((uint16_t)usage.usage_page)<<8) | ((usage.usage_max) & 0xff);
        return 0;
    }
    return -1;
}

static void hid_list_entries(struct hid *h)
{
    uint16_t usage_min, usage_max;

    HIDDEBUG("**********************************");
    HIDDEBUG("**********************************");
    if (h->reports == NULL) {
        HIDDEBUG("No entries");
    } else {
        hid_report_t *rep = h->reports;
        while (rep) {
            HIDDEBUG("Report %d size %d", rep->id, rep->size);
            hid_field_t *field = rep->fields;
            if (!field) {
                HIDDEBUG(" > No fiels");
            } else {
                while (field) {
                    HIDDEBUG(" > Field offset %d size %d count %d",
                             field->report_offset,
                             field->report_size,
                             field->report_count);
                    int i;
                    for (i=0;i<field->report_count;i++) {

                        if (hid__field_get_usage(field, i, &usage_min, &usage_max)<0) {
                            HIDERROR("Cannot get field usage");
                            return;
                        }
                        if (usage_min == usage_max) {
                            HIDDEBUG("  [%d] usage 0x%04x (%s)", i, usage_min,
                                     hid_usage_find(usage_min));
                        } else {
                            HIDDEBUG("  [%d] usage 0x%04x -> 0x%04x", i, usage_min, usage_max);
                        }
                    }
                    field = field->next;
                }
            }
            rep = rep->next;
        }
    }
}

int hid__extract_field_aligned(const hid_field_t *field, uint8_t index, const uint8_t *data, uint8_t *dest)
{
    unsigned start_bit = field->report_offset + ((unsigned)index * field->report_size);

    unsigned start = start_bit >> 3;
    unsigned end = (start_bit+field->report_size-1) >> 3;
    if (start!=end)
        return -1;
    uint8_t val = data[start];
    unsigned rem = start_bit & 0x7;
    unsigned mask = (1U<<(field->report_size))-1;
    val >>= rem;
    val &= mask;
    *dest = val;
    return 0;
}


/**
 * \ingroup hid
 * \brief Parse a HID descritor block
 */
struct hid * hid__parse(const uint8_t *hiddesc, int len)
{
    struct hid *h = zalloc(sizeof(struct hid));

    h->current_collection = -1;

    int r = 0;

    while (len>0) {
        uint8_t v = *hiddesc++;
        len--;
//        printf("BYTE 0x%02x\n", v);
        uint8_t tag_type;
        uint8_t tlen = v & 0x3;
        /*
         0: no data
         1: 1 byte
         2: 2 bytes
         3: 4 bytes
         */

        if (tlen==3)
            tlen++;

        if ((v & 0xF0)==0xF0) {
            HIDERROR("Long format HID not supported!");
            r = -1;
            break;
        }

        HIDDEBUG("%02x: Tag type: %d\n", v, (v>>2)&3);

        v>>=2;
        tag_type = v & 0x3;
        switch (tag_type) {
        case 0:
            r = parse_hid_main_entry(h, v, hiddesc, tlen);
            break;
        case 1:
            r = parse_hid_global_entry(h, v, hiddesc, tlen);
            break;
        case 2:
            r = parse_hid_local_entry(h, v, hiddesc, tlen);
            break;
        case 3:
            HIDDEBUG(">> RESERVED %02x",v);
            r = -1; // Reserved
            break;
        }

        if (r<0) {
            HIDDEBUG("Error parsing");
            break;
        }
        len-=(tlen);
        hiddesc+=(tlen);
        HIDDEBUG("Len %d",len);
    }

    if (len<0) {
        HIDERROR("Report desctriptor too short");
        r=-1;
    }

    if (r==0) {
        hid_list_entries(h);
        return h;
    }
    return NULL;
}

static void hid_free_report(hid_report_t *r)
{
    while (r->fields) {
        hid_field_t *n = r->fields->next;
        free(r->fields);
        r->fields = n;
    }
}

void hid__free(struct hid *h)
{
    while (h->reports) {
        hid_report_t *r = h->reports->next;
        hid_free_report(h->reports);
        h->reports = r;
    }
    free(h);
}

#ifdef TESTHIDPARSER

int main()
{
    const char hid[] = {
        0x05,0x01,0x09,0x04,0xa1,0x01,0xa1,0x02,0x75,0x08,0x95,0x05,0x15,0x00,
        0x26,0xff,0x00,0x35,0x00,0x46,0xff,0x00,0x09,0x32,0x09,0x35,0x09,0x30,
        0x09,0x31,0x09,0x00,0x81,0x02,0x75,0x04,0x95,0x01,0x25,0x07,0x46,0x3b,
        0x01,0x65,0x14,0x09,0x39,0x81,0x42,0x65,0x00,0x75,0x01,0x95,0x0c,0x25,
        0x01,0x45,0x01,0x05,0x09,0x19,0x01,0x29,0x0c,0x81,0x02,0x06,0x00,0xff,
        0x75,0x01,0x95,0x08,0x25,0x01,0x45,0x01,0x09,0x01,0x81,0x02,0xc0,0xa1,
        0x02,0x75,0x08,0x95,0x04,0x46,0xff,0x00,0x26,0xff,0x00,0x09,0x02,0x91,
        0x02,0xc0,0xc0
    };

    /*
     0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
     0x09, 0x04,        // Usage (Joystick)
     0xA1, 0x01,        // Collection (Application)
     0xA1, 0x02,        //   Collection (Logical)

     0x75, 0x08,        //     Report Size (8)
     0x95, 0x05,        //     Report Count (5)
     0x15, 0x00,        //     Logical Minimum (0)
     0x26, 0xFF, 0x00,  //     Logical Maximum (255)
     0x35, 0x00,        //     Physical Minimum (0)
     0x46, 0xFF, 0x00,  //     Physical Maximum (255)
     0x09, 0x32,        //     Usage (Z)
     0x09, 0x35,        //     Usage (Rz)
     0x09, 0x30,        //     Usage (X)
     0x09, 0x31,        //     Usage (Y)
     0x09, 0x00,        //     Usage (Undefined)
     0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

     0x75, 0x04,        //     Report Size (4)
     0x95, 0x01,        //     Report Count (1)
     0x25, 0x07,        //     Logical Maximum (7)
     0x46, 0x3B, 0x01,  //     Physical Maximum (315)
     0x65, 0x14,        //     Unit (System: English Rotation, Length: Centimeter)
     0x09, 0x39,        //     Usage (Hat switch)
     0x81, 0x42,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
     0x65, 0x00,        //     Unit (None)

     0x75, 0x01,        //     Report Size (1)
     0x95, 0x0C,        //     Report Count (12)
     0x25, 0x01,        //     Logical Maximum (1)
     0x45, 0x01,        //     Physical Maximum (1)

     0x05, 0x09,        //     Usage Page (Button)
     0x19, 0x01,        //     Usage Minimum (0x01)
     0x29, 0x0C,        //     Usage Maximum (0x0C)
     0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)

     0x75, 0x01,        //     Report Size (1)
     0x95, 0x08,        //     Report Count (8)
     0x25, 0x01,        //     Logical Maximum (1)
     0x45, 0x01,        //     Physical Maximum (1)
     0x09, 0x01,        //     Usage (0x01)
     0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0xC0,              //   End Collection
     0xA1, 0x02,        //   Collection (Logical)
     0x75, 0x08,        //     Report Size (8)
     0x95, 0x04,        //     Report Count (4)
     0x46, 0xFF, 0x00,  //     Physical Maximum (255)
     0x26, 0xFF, 0x00,  //     Logical Maximum (255)
     0x09, 0x02,        //     Usage (0x02)
     0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
     0xC0,              //   End Collection
     0xC0,              // End Collection

     // 101 bytes
     */


    uint8_t xbox_pad_report_desc_bin[] = {
        0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x05, 0x01, 0x09, 0x3a, 0xa1, 0x02,
        0x75, 0x08, 0x95, 0x01, 0x81, 0x01, 0x75, 0x08, 0x95, 0x01, 0x05, 0x01,
        0x09, 0x3b, 0x81, 0x01, 0x05, 0x01, 0x09, 0x01, 0xa1, 0x00, 0x75, 0x01,
        0x15, 0x00, 0x25, 0x01, 0x35, 0x00, 0x45, 0x01, 0x95, 0x04, 0x05, 0x01,
        0x09, 0x90, 0x09, 0x91, 0x09, 0x93, 0x09, 0x92, 0x81, 0x02, 0xc0, 0x75,
        0x01, 0x15, 0x00, 0x25, 0x01, 0x35, 0x00, 0x45, 0x01, 0x95, 0x04, 0x05,
        0x09, 0x19, 0x07, 0x29, 0x0a, 0x81, 0x02, 0x75, 0x01, 0x95, 0x08, 0x81,
        0x01, 0x75, 0x08, 0x15, 0x00, 0x26, 0xff, 0x00, 0x35, 0x00, 0x46, 0xff,
        0x00, 0x95, 0x06, 0x05, 0x09, 0x19, 0x01, 0x29, 0x06, 0x81, 0x02, 0x75,
        0x08, 0x15, 0x00, 0x26, 0xff, 0x00, 0x35, 0x00, 0x46, 0xff, 0x00, 0x95,
        0x02, 0x05, 0x01, 0x09, 0x32, 0x09, 0x35, 0x81, 0x02, 0x75, 0x10, 0x16,
        0x00, 0x80, 0x26, 0xff, 0x7f, 0x36, 0x00, 0x80, 0x46, 0xff, 0x7f, 0x05,
        0x01, 0x09, 0x01, 0xa1, 0x00, 0x95, 0x02, 0x05, 0x01, 0x09, 0x30, 0x09,
        0x31, 0x81, 0x02, 0xc0, 0x05, 0x01, 0x09, 0x01, 0xa1, 0x00, 0x95, 0x02,
        0x05, 0x01, 0x09, 0x33, 0x09, 0x34, 0x81, 0x02, 0xc0, 0xc0, 0x05, 0x01,
        0x09, 0x3a, 0xa1, 0x02, 0x75, 0x08, 0x95, 0x01, 0x91, 0x01, 0x75, 0x08,
        0x95, 0x01, 0x05, 0x01, 0x09, 0x3b, 0x91, 0x01, 0x75, 0x08, 0x95, 0x01,
        0x91, 0x01, 0x75, 0x08, 0x15, 0x00, 0x26, 0xff, 0x00, 0x35, 0x00, 0x46,
        0xff, 0x00, 0x95, 0x01, 0x06, 0x00, 0xff, 0x09, 0x01, 0x91, 0x02, 0x75,
        0x08, 0x95, 0x01, 0x91, 0x01, 0x75, 0x08, 0x15, 0x00, 0x26, 0xff, 0x00,
        0x35, 0x00, 0x46, 0xff, 0x00, 0x95, 0x01, 0x06, 0x00, 0xff, 0x09, 0x02,
        0x91, 0x02, 0xc0, 0xc0
    };

    uint8_t keyboard_desc_bin[] = {
        0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x05, 0x07,
        0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01,
        0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01,
        0x75, 0x08, 0x81, 0x01, 0x95, 0x05, 0x75, 0x01,
        0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
        0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0x95, 0x06,
        0x75, 0x08, 0x15, 0x00, 0x26, 0xff, 0x00, 0x05,
        0x07, 0x19, 0x00, 0x2a, 0xff, 0x00, 0x81, 0x00,
        0xc0
    };

    const uint8_t lenovo_mouse_hid[] = {
        0x05,0x01,0x09,0x02,0xA1,0x01,0x09,0x01,0xA1,0x00,0x05,0x09,0x19,0x01,0x29,0x03,
        0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x03,0x81,0x02,0x75,0x05,0x95,0x01,0x81,0x03,
        0x06,0x00,0xFF,0x09,0x40,0x95,0x02,0x75,0x08,0x15,0x81,0x25,0x7F,0x81,0x02,0x05,
        0x01,0x09,0x38,0x15,0x81,0x25,0x7F,0x75,0x08,0x95,0x01,0x81,0x06,0x09,0x30,0x09,
        0x31,0x16,0x01,0x80,0x26,0xFF,0x7F,0x75,0x10,0x95,0x02,0x81,0x06,0xC0,0xC0
    };

    const uint8_t apple_kbd_hid[]  = {
        0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
        0x09, 0x06,        // Usage (Keyboard)
        0xA1, 0x01,        // Collection (Application)
        0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
        0x19, 0xE0,        //   Usage Minimum (0xE0)
        0x29, 0xE7,        //   Usage Maximum (0xE7)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x01,        //   Logical Maximum (1)
        0x75, 0x01,        //   Report Size (1)
        0x95, 0x08,        //   Report Count (8)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x95, 0x01,        //   Report Count (1)
        0x75, 0x08,        //   Report Size (8)
        0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x05, 0x08,        //   Usage Page (LEDs)
        0x19, 0x01,        //   Usage Minimum (Num Lock)
        0x29, 0x05,        //   Usage Maximum (Kana)
        0x95, 0x05,        //   Report Count (5)
        0x75, 0x01,        //   Report Size (1)
        0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x95, 0x01,        //   Report Count (1)
        0x75, 0x03,        //   Report Size (3)
        0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
        0x19, 0x00,        //   Usage Minimum (0x00)
        0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
        0x95, 0x05,        //   Report Count (5)
        0x75, 0x08,        //   Report Size (8)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x05, 0xFF,        //   Usage Page (Reserved 0xFF)
        0x09, 0x03,        //   Usage (0x03)
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x01,        //   Report Count (1)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0xC0,              // End Collection
    };
    /*
     0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
     0x09, 0x05,        // Usage (Game Pad)
     0xA1, 0x01,        // Collection (Application)
     0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
     0x09, 0x3A,        //   Usage (Counted Buffer)
     0xA1, 0x02,        //   Collection (Logical)
     0x75, 0x08,        //     Report Size (8)
     0x95, 0x01,        //     Report Count (1)
     0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0x75, 0x08,        //     Report Size (8)
     0x95, 0x01,        //     Report Count (1)
     0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
     0x09, 0x3B,        //     Usage (Byte Count)
     0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
     0x09, 0x01,        //     Usage (Pointer)
     0xA1, 0x00,        //     Collection (Physical)
     0x75, 0x01,        //       Report Size (1)
     0x15, 0x00,        //       Logical Minimum (0)
     0x25, 0x01,        //       Logical Maximum (1)
     0x35, 0x00,        //       Physical Minimum (0)
     0x45, 0x01,        //       Physical Maximum (1)
     0x95, 0x04,        //       Report Count (4)
     0x05, 0x01,        //       Usage Page (Generic Desktop Ctrls)
     0x09, 0x90,        //       Usage (D-pad Up)
     0x09, 0x91,        //       Usage (D-pad Down)
     0x09, 0x93,        //       Usage (D-pad Left)
     0x09, 0x92,        //       Usage (D-pad Right)
     0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0xC0,              //     End Collection
     0x75, 0x01,        //     Report Size (1)
     0x15, 0x00,        //     Logical Minimum (0)
     0x25, 0x01,        //     Logical Maximum (1)
     0x35, 0x00,        //     Physical Minimum (0)
     0x45, 0x01,        //     Physical Maximum (1)
     0x95, 0x04,        //     Report Count (4)
     0x05, 0x09,        //     Usage Page (Button)
     0x19, 0x07,        //     Usage Minimum (0x07)
     0x29, 0x0A,        //     Usage Maximum (0x0A)
     0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0x75, 0x01,        //     Report Size (1)
     0x95, 0x08,        //     Report Count (8)
     0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0x75, 0x08,        //     Report Size (8)
     0x15, 0x00,        //     Logical Minimum (0)
     0x26, 0xFF, 0x00,  //     Logical Maximum (255)
     0x35, 0x00,        //     Physical Minimum (0)
     0x46, 0xFF, 0x00,  //     Physical Maximum (255)
     0x95, 0x06,        //     Report Count (6)
     0x05, 0x09,        //     Usage Page (Button)
     0x19, 0x01,        //     Usage Minimum (0x01)
     0x29, 0x06,        //     Usage Maximum (0x06)
     0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0x75, 0x08,        //     Report Size (8)
     0x15, 0x00,        //     Logical Minimum (0)
     0x26, 0xFF, 0x00,  //     Logical Maximum (255)
     0x35, 0x00,        //     Physical Minimum (0)
     0x46, 0xFF, 0x00,  //     Physical Maximum (255)
     0x95, 0x02,        //     Report Count (2)
     0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
     0x09, 0x32,        //     Usage (Z)
     0x09, 0x35,        //     Usage (Rz)
     0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0x75, 0x10,        //     Report Size (16)
     0x16, 0x00, 0x80,  //     Logical Minimum (-32768)
     0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
     0x36, 0x00, 0x80,  //     Physical Minimum (-32768)
     0x46, 0xFF, 0x7F,  //     Physical Maximum (32767)
     0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
     0x09, 0x01,        //     Usage (Pointer)
     0xA1, 0x00,        //     Collection (Physical)
     0x95, 0x02,        //       Report Count (2)
     0x05, 0x01,        //       Usage Page (Generic Desktop Ctrls)
     0x09, 0x30,        //       Usage (X)
     0x09, 0x31,        //       Usage (Y)
     0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0xC0,              //     End Collection
     0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
     0x09, 0x01,        //     Usage (Pointer)
     0xA1, 0x00,        //     Collection (Physical)
     0x95, 0x02,        //       Report Count (2)
     0x05, 0x01,        //       Usage Page (Generic Desktop Ctrls)
     0x09, 0x33,        //       Usage (Rx)
     0x09, 0x34,        //       Usage (Ry)
     0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
     0xC0,              //     End Collection
     0xC0,              //   End Collection
     0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
     0x09, 0x3A,        //   Usage (Counted Buffer)
     0xA1, 0x02,        //   Collection (Logical)
     0x75, 0x08,        //     Report Size (8)
     0x95, 0x01,        //     Report Count (1)
     0x91, 0x01,        //     Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
     0x75, 0x08,        //     Report Size (8)
     0x95, 0x01,        //     Report Count (1)
     0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
     0x09, 0x3B,        //     Usage (Byte Count)
     0x91, 0x01,        //     Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
     0x75, 0x08,        //     Report Size (8)
     0x95, 0x01,        //     Report Count (1)
     0x91, 0x01,        //     Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
     0x75, 0x08,        //     Report Size (8)
     0x15, 0x00,        //     Logical Minimum (0)
     0x26, 0xFF, 0x00,  //     Logical Maximum (255)
     0x35, 0x00,        //     Physical Minimum (0)
     0x46, 0xFF, 0x00,  //     Physical Maximum (255)
     0x95, 0x01,        //     Report Count (1)
     0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
     0x09, 0x01,        //     Usage (0x01)
     0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
     0x75, 0x08,        //     Report Size (8)
     0x95, 0x01,        //     Report Count (1)
     0x91, 0x01,        //     Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
     0x75, 0x08,        //     Report Size (8)
     0x15, 0x00,        //     Logical Minimum (0)
     0x26, 0xFF, 0x00,  //     Logical Maximum (255)
     0x35, 0x00,        //     Physical Minimum (0)
     0x46, 0xFF, 0x00,  //     Physical Maximum (255)
     0x95, 0x01,        //     Report Count (1)
     0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
     0x09, 0x02,        //     Usage (0x02)
     0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
     0xC0,              //   End Collection
     0xC0,              // End Collection

     // 256 bytes
     */

    /* Keyboard
     05 01 Usage Page (Generic Desktop Ctrls)
     09 06 Usage: Keyboard
     a1 01 Collection Appliation
     05 07 Usage Page: Keyboard/keypad
     19 e0 Usage minimum
     29 e7 Usage maximum
     15 00 Logical minimum
     25 01 Logical maximum
     75 01 Report size
     95 08 Report count
     81 02 Input (variable)            ; Modifier keys
     95 01 Report count
     75 08 Report size
     81 01 Input (constant)            ; Reserved field
     95 05 Report count
     75 01 Report size
     05 08 Usage (LEDS)
     19 01 Usage minimum
     29 05 Usage maximum
     91 02 Input (variable)
     95 01 Report count
     75 03 Report size
     91 01 Output
     95 06 Report count
     75 08 Report size
     15 00 Logical minimum
     26 ff 00 Logical maximum
     05 07 Usage (Keyboard)
     19 00 Usage minimum
     2a ff 00 Usage maximum
     81 00 Input (array)
     c0 End collection
     */

    const uint8_t rickard1_hid[] = {
        0x05,0x01,0x09,0x02,0xA1,0x01,0x85,0x02,0x09,0x01,0xA1,0x00,0x05,0x09,0x19,0x01
        ,0x29,0x05,0x15,0x00,0x25,0x01,0x95,0x05,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x03
        ,0x81,0x01,0x05,0x01,0x09,0x30,0x09,0x31,0x15,0x81,0x25,0x7F,0x95,0x02,0x75,0x08
        ,0x81,0x06,0x09,0x38,0x15,0x81,0x25,0x7F,0x95,0x01,0x75,0x08,0x81,0x06,0x05,0x0C
        ,0x0A,0x38,0x02,0x15,0x81,0x25,0x7F,0x95,0x01,0x75,0x08,0x81,0x06,0xC0,0xC0,0x05
        ,0x01,0x09,0x80,0xA1,0x01,0x85,0x03,0x16,0x01,0x00,0x26,0x03,0x00,0x1A,0x81,0x00
        ,0x2A,0x83,0x00,0x75,0x10,0x95,0x01,0x81,0x00,0xC0,0x05,0x0C,0x09,0x01,0xA1,0x01
        ,0x85,0x04,0x16,0x01,0x00,0x26,0x9C,0x02,0x1A,0x01,0x00,0x2A,0x9C,0x02,0x75,0x10
        ,0x95,0x01,0x81,0x00,0xC0,0x05,0x01,0x09,0x06,0xA1,0x01,0x85,0x05,0x05,0x07,0x19
        ,0xE0,0x29,0xE7,0x15,0x00,0x25,0x01,0x95,0x08,0x75,0x01,0x81,0x02,0x05,0x08,0x19
        ,0x01,0x29,0x05,0x95,0x05,0x75,0x01,0x91,0x02,0x95,0x01,0x75,0x03,0x91,0x01,0x05
        ,0x07,0x19,0x00,0x29,0xEF,0x15,0x00,0x25,0x01,0x95,0xF0,0x75,0x01,0x81,0x02,0xC0

    };
    const uint8_t rickard2_hid[] = {

        0x05,0x01,0x09,0x06,0xA1,0x01,0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,0x25,0x01
        ,0x95,0x08,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x08,0x81,0x01,0x05,0x08,0x19,0x01
        ,0x29,0x05,0x95,0x05,0x75,0x01,0x91,0x02,0x95,0x01,0x75,0x03,0x91,0x01,0x05,0x07
        ,0x19,0x00,0x29,0xFF,0x15,0x00,0x26,0xFF,0x00,0x95,0x06,0x75,0x08,0x81,0x00,0xC0

    };

    static uint8_t joy_hid[]  =
    {
        0x05, 0x01, // Item(Global): Usage Page, data= [ 0x01 ] 1 Generic Desktop Controls
        0x09, 0x04, // Usage, data= [ 0x04 ] 4 Joystick
        0xA1, 0x01, // Collection, data= [ 0x01 ] 1 Application
        0xA1, 0x02, // Collection, data= [ 0x02 ] 2 Logical
        0x85, 0x01, // Report ID (1)
        0x75, 0x02, // Report Size 2
        0x95, 0x02, // Report Count 2
        0x15, 0xFF, // Logical Minimum (-1)
        0x25, 0x01, // Logical Maximum (1)
        0x35, 0xFF, // Physical Minimum -1
        0x45, 0x01, // Physical Maximum (1)
        0x09, 0x30, // Usage, data= [ 0x30 ] 48 Direction-X
        0x09, 0x31, // Usage, data= [ 0x31 ] 49 Direction-Y
        0x81, 0x02, // Input, data= [ 0x02 ] 2 Data Variable Absolute No_Wrap Linear Preferred_State No_Null_Position Non_Volatile Bitfield

        0x75, 0x01, // Report Size, data= [ 0x01 ] 1
        0x95, 0x03, // Report Count, data= [ 0x03 ] 3
        0x15, 0x00, // Logical Minimum (0)
        0x25, 0x01, // Logical Maximum, data= [ 0x01 ] 1
        0x35, 0x00, // Physical Minimum 0
        0x45, 0x01, // Physical Maximum, data= [ 0x01 ] 1
        0x05, 0x09, // Usage Page, data= [ 0x09 ] 9 Buttons
        0x19, 0x01, // Usage Minimum, data= [ 0x01 ] 1 Button 1 (Primary)
        0x29, 0x03, // Usage Maximum, data= [ 0x03 ] 1 Button 3
        0x81, 0x02, // Input, data= [ 0x02 ] 2 Data Variable Absolute No_Wrap Linear Preferred_State No_Null_Position Non_Volatile Bitfield

        0x95, 0x01, // Report Count 1
        0x81, 0x03, // Input, const
        0xC0,       // End collection
        0xC0,  // End collection
        0xA1, 0x01, // Collection, data= [ 0x01 ] 1 Application
        0x05, 0x01, // Usage Page, data= [ 0x09 ] 1 Generic Desktop Controls
        0xA1, 0x02, // Collection, data= [ 0x02 ] 2 Logical
        0x85, 0x02, // Report ID (2)
        0x75, 0x02, // Report Size 2
        0x95, 0x02, // Report Count 2
        0x15, 0xFF, // Logical Minimum (-1)
        0x25, 0x01, // Logical Maximum (1)
        0x35, 0xFF, // Physical Minimum -1
        0x45, 0x01, // Physical Maximum (1)
        0x09, 0x30, // Usage, data= [ 0x30 ] 48 Direction-X
        0x09, 0x31, // Usage, data= [ 0x31 ] 49 Direction-Y
        0x81, 0x02, // Input, data= [ 0x02 ] 2 Data Variable Absolute No_Wrap Linear Preferred_State No_Null_Position Non_Volatile Bitfield

        0x75, 0x01, // Report Size, data= [ 0x01 ] 1
        0x95, 0x03, // Report Count, data= [ 0x03 ] 3
        0x15, 0x00, // Logical Minimum (0)
        0x25, 0x01, // Logical Maximum, data= [ 0x01 ] 1
        0x35, 0x00, // Physical Minimum 0
        0x45, 0x01, // Physical Maximum, data= [ 0x01 ] 1
        0x05, 0x09, // Usage Page, data= [ 0x09 ] 9 Buttons
        0x19, 0x01, // Usage Minimum, data= [ 0x01 ] 1 Button 1 (Primary)
        0x29, 0x03, // Usage Maximum, data= [ 0x03 ] 1 Button 3
        0x81, 0x02, // Input, data= [ 0x02 ] 2 Data Variable Absolute No_Wrap Linear Preferred_State No_Null_Position Non_Volatile Bitfield

        0x95, 0x01, // Report Count 1
        0x81, 0x03, // Input, const
        0xC0,       // End collection
        0xC0  // End collection

    };

#if 0
    if (hid_parse(hid,sizeof(hid))<0) {
        printf("ERROR\n");
    } else {
        printf("OK\n");
    }
#endif

#if 0
    if (hid_parse(xbox_pad_report_desc_bin,sizeof(xbox_pad_report_desc_bin))<0) {
        printf("ERROR\n");
    } else {
        printf("OK\n");
    }
#endif

#if 0
    if (hid_parse(keyboard_desc_bin,sizeof(keyboard_desc_bin))==NULL) {
        printf("ERROR\n");
    } else {
        printf("OK\n");
    }
#endif

#if 0
    if (hid_parse(lenovo_mouse_hid,sizeof(lenovo_mouse_hid))==NULL) {
        printf("ERROR\n");
    } else {
        printf("OK\n");
    }
#endif
#if 0
    if (hid_parse(apple_kbd_hid,sizeof(apple_kbd_hid))==NULL) {
        printf("ERROR\n");
    } else {
        printf("OK\n");
    }
#endif

#if 0
    if (hid_parse(rickard2_hid,sizeof(rickard2_hid))==NULL) {
        printf("ERROR\n");
    } else {
        printf("OK\n");
    }
#endif

#if 1
    if (hid_parse(joy_hid,sizeof(joy_hid))==NULL) {
        printf("ERROR\n");
    } else {
        printf("OK\n");
    }
#endif
}



#endif

