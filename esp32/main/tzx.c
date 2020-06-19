#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tzx.h"
#include "byteops.h"

#ifdef __linux__

#define do_log(m, tag,x...) do { \
    printf("%s %s ", m,tag); \
    printf(x);\
    printf("\n"); \
} while (0)

#define ESP_LOGI(tag,x...) do_log("I", tag, x)
#define ESP_LOGE(tag,x...) do_log("E", tag, x)
#define ESP_LOGW(tag,x...) do_log("W", tag, x)
#define ESP_LOGD(tag,x...) 

#else
#include "esp_log.h"
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define TAG "TZX"



static int tzx__check(struct tzx *t, const uint8_t **data, int *len, int needed)
{
    //int have;
    while (t->tzxbufptr<needed && (*len>0) ) {
        t->tzxbuf[t->tzxbufptr++] = *(*data);
        (*len)--;
        (*data)++;
    }
    if (t->tzxbufptr==needed) {
        t->tzxbufptr = 0;
        return 0;
    }
    return -1;
}


static const uint8_t tzxsig[8] = {
    'Z','X','T','a','p','e','!', 0x1A };

void tzx__chunk(struct tzx *t, const uint8_t *data, int len)
{
    uint32_t val, pause;
    int alen;

#define NEED(x) if (tzx__check(t, &data, &len, x)<0) return;

    if (len==0) {
        tzx__finished_callback();
        return;
    }
    //ESP_LOGI(TAG, "Parse chunk len %d", len);
    do {
        switch (t->state) {
        case HEADER:
            NEED(10);
            if (memcmp(t->tzxbuf, tzxsig, sizeof(tzxsig))!=0) {
                ESP_LOGE(TAG,"Invalid header\n");
                t->state = INVALID;
                break;
            }
            ESP_LOGI(TAG,"TZX version: %d.%d",
                   t->tzxbuf[8],
                   t->tzxbuf[9]);
            t->state = BLOCK;
            break;

        case BLOCK:
            NEED(1);
            ESP_LOGI(TAG,"Block: 0x%02x",t->tzxbuf[0]);
            switch (t->tzxbuf[0]) {
            case 0x30:
                ESP_LOGI(TAG,"Text block");
                t->state = DESCRIPTION;
                break;
            case 0x32:
                ESP_LOGI(TAG,"Archive info block");
                t->state = ARCHIVEINFO;
                break;
            case 0x10:
                // Standard data block.
                t->state = STANDARD;
                break;
            case 0x11:
                t->state = TURBOSPEED;
                break;
            case 0x12:
                t->state = PURETONE;
                break;
            case 0x13:
                t->state = PULSES;
                break;
            case 0x14:
                t->state = PUREDATA;
                break;
            case 0x21:
                t->state = GROUP;
                break;
            case 0x22:
                //t->state = GROUP_END;
                break;


            default:
                ESP_LOGE(TAG,"Unknown block type %02x", t->tzxbuf[0]);
                t->state = INVALID;
                break;
            }
            break;
        case GROUP:
            NEED(1);
            t->datachunk  = t->tzxbuf[0];
            t->state = IGNOREDATA;
            break;

        case DESCRIPTION:
            NEED(1);
            t->datachunk  = t->tzxbuf[0];
            t->state = IGNOREDATA;
            //t->dataptr = NULL;
            break;

        case ARCHIVEINFO:
            NEED(2);

            t->datachunk = extractle16(t->tzxbuf);
            t->state = IGNOREDATA;

            break;
        case PURETONE:
            NEED(4);
            tzx__tone_callback( extractle16(&t->tzxbuf[0]), extractle16(&t->tzxbuf[2]) );
            t->state = BLOCK;
            break;

        case PULSES:
            NEED(1);
            t->state = PULSEDATA;
            t->pulsecount = 0;
            t->pulses = t->tzxbuf[0];
            ESP_LOGI(TAG, "Number of pulses: %d", t->pulses);
            break;

        case PULSEDATA:
            NEED(2);
            val = extractle16(&t->tzxbuf[0]);
            t->pulse_data[ t->pulsecount ] = val;
            t->pulsecount++;

            ESP_LOGI(TAG, "Pulse width (%d): %d", t->pulsecount-1, val);

            if (t->pulsecount == t->pulses) {
                tzx__pulse_callback(t->pulses, &t->pulse_data[0]);
                t->state = BLOCK;
            } 

            break;
        case PUREDATA:
            NEED(10);

            t->datachunk = extractle24(&t->tzxbuf[7]) ;
            tzx__pure_data_callback( extractle16(&t->tzxbuf[0]),
                                    extractle16(&t->tzxbuf[2]),
                                    t->datachunk,
                                    extractle16(&t->tzxbuf[5]),
                                    t->tzxbuf[4]);

            t->state = RAWDATA;
            break;

        case RAWDATA:

            alen = MIN((int)len, (int)t->datachunk);

            tzx__data_callback(data, alen);

            t->datachunk-=alen;
            len-=alen;
            data+=alen;

            if (t->datachunk==0) {
                tzx__data_finished_callback();
                t->state = BLOCK;
            }

            break;

        case IGNOREDATA:
            alen = MIN((int)len, (int)t->datachunk);
            ESP_LOGI(TAG," SKIP %d alen %d", t->datachunk, alen);
            t->datachunk-=alen;
            len-=alen;
            data+=alen;
            if (t->datachunk==0)
                t->state = BLOCK;

            break;

        case STANDARD:
            NEED(4);
            //dump(t->tzxbuf,4);
            pause = extractle16(&t->tzxbuf[0]);

            //ESP_LOGE(TAG,("Pause: %d\n", pause);
            val = extractle16(&t->tzxbuf[2]);
            //ESP_LOGE(TAG,("Len: %d\n", val);

            t->datachunk = val;
            t->state = RAWDATA;
            ESP_LOGI(TAG,"Standard block ahead, %d bytes", val);

            tzx__standard_block_callback(val, pause);

            break;

        case TURBOSPEED:
            NEED(18);
            uint16_t pilot, sync0, sync1, pulse0, pulse1, pilot_len, gap_len;
            uint32_t data_len;
            //uint8_t lastbyte;

            pilot = extractle16(&t->tzxbuf[0]);

            sync0 = extractle16(&t->tzxbuf[2]);
            sync1 = extractle16(&t->tzxbuf[4]);
            pulse0 = extractle16(&t->tzxbuf[6]);
            pulse1 = extractle16(&t->tzxbuf[8]);
            pilot_len = extractle16(&t->tzxbuf[0x0a]);
            t->lastbytesize = t->tzxbuf[0x0c];
            gap_len = extractle16(&t->tzxbuf[0x0d]);
            data_len = extractle24(&t->tzxbuf[0x0f]);

            //ESP_LOGE(TAG,("Data len: %d (last byte %d bits)\n", data_len, t->lastbytesize);
            ESP_LOGI(TAG,"Turbo-speed block ahead, %d bytes", data_len);
            t->datachunk = data_len;
            t->state = RAWDATA;
            tzx__turbo_block_callback(pilot, sync0, sync1, pulse0, pulse1, pilot_len, gap_len, data_len, t->lastbytesize);

            break;


        default:
            ESP_LOGE(TAG,"ERROR STATE\n");
#ifdef __linux__
            exit(-1);
#endif
            break;
        }
    } while (len>0);
}

void tzx__init(struct tzx *t)
{
    t->tzxbufptr = 0;
    t->state = HEADER;
}

#ifdef TESTTZX
int main(int argc, char **argv)
{
    char buf[1];
    int r;
    struct tzx t;
    t.tzxbufptr = 0;
    t.state = HEADER;

    int fd = open(argv[1],O_RDONLY);
    if (fd<0)
        return -1;
    do {
        r = read(fd,&buf,sizeof(buf));
        if (r>0)
            tzx__chunk(&t, buf,sizeof(buf));
    } while (r>0);
    close(fd);

}

void tzx__standard_block_callback(uint16_t length, uint16_t pause_after)
{
}
void tzx__turbo_block_callback(uint16_t pilot, uint16_t sync0, uint16_t sync1, uint16_t pulse0, uint16_t pulse1, uint16_t pilot_len, uint32_t data_len,
                               uint8_t last_byte_len)
{
}

void tzx__pure_data_callback(uint16_t pulse0, uint16_t pulse1, uint32_t data_len, uint16_t gap,
                             uint8_t last_byte_len)
{
}

void tzx__data_callback(const uint8_t *data, int len)
{
}
void tzx__tone_callback(uint16_t t_states, uint16_t count)
{
}
void tzx__pulse_callback(uint8_t count, const uint16_t *t_states /* these are words */)
{
}
#endif
