#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tzx.h"
#include "esp_log.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define TAG "TZX"

static int tzx_check(struct tzx *t, const uint8_t **data, int *len, int needed)
{
    int have;
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

static inline uint32_t extractle16(const uint8_t *source)
{
    return (((uint16_t)source[1])<<8) + ((uint16_t)source[0]);
}

static inline uint32_t extractle24(const uint8_t *source)
{
    return (((uint32_t)source[2])<<16) +
        (((uint32_t)source[1])<<8) + ((uint32_t)source[0]);
}

static const uint8_t tzxsig[8] = {
    'Z','X','T','a','p','e','!', 0x1A };

void tzx__chunk(struct tzx *t, const uint8_t *data, int len)
{
    uint32_t val, pause;
    int alen;

#define NEED(x) if (tzx_check(t, &data, &len, x)<0) return;

    do {
        switch (t->state) {
        case HEADER:
            NEED(10);
            ESP_LOGE(TAG,"Header\n");
            if (memcmp(t->tzxbuf, tzxsig, sizeof(tzxsig))!=0) {
                ESP_LOGE(TAG,"Invalid header\n");
                t->state = INVALID;
                break;
            }
            ESP_LOGI(TAG,"TZX version: %d %d\n",
                   t->tzxbuf[8],
                   t->tzxbuf[9]);
            t->state = BLOCK;
            break;
        case BLOCK:
            NEED(1);
            //ESP_LOGE(TAG,("%02x\n",t->tzxbuf[0]);
            switch (t->tzxbuf[0]) {
            case 0x30:
                ESP_LOGI(TAG,"Text block\n");
                t->state = DESCRIPTION;
                break;
            case 0x10:
                // Standard data block.
                t->state = STANDARD;
                break;
            case 0x11:
                t->state = TURBOSPEED;
                break;

            default:
                ESP_LOGE(TAG,"Unknown block type %02x\n", t->tzxbuf[0]);
                t->state = INVALID;
                break;
            }
            break;

        case DESCRIPTION:
            NEED(1);
            t->datachunk  = t->tzxbuf[0];
            t->state = RAWDATA;
            //t->dataptr = NULL;
            break;

        case RAWDATA:

            alen = MIN(len, t->datachunk);

            tzx__data_callback(data, alen);

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
            tzx__standard_block_callback(val, pause);

            break;

        case TURBOSPEED:
            NEED(18);
            uint16_t pilot, sync0, sync1, pulse0, pulse1;
            uint32_t data_len;
            uint8_t lastbyte;

            pilot = extractle16(&t->tzxbuf[0]);

            sync0 = extractle16(&t->tzxbuf[2]);
            sync1 = extractle16(&t->tzxbuf[4]);
            pulse0 = extractle16(&t->tzxbuf[6]);
            pulse1 = extractle16(&t->tzxbuf[8]);
            data_len = extractle24(&t->tzxbuf[0x0f]);

            t->lastbytesize = t->tzxbuf[0x0c];
            //ESP_LOGE(TAG,("Data len: %d (last byte %d bits)\n", data_len, t->lastbytesize);
            t->datachunk = data_len;
            t->state = RAWDATA;
            tzx__turbo_block_callback(pilot, sync0, sync1, pulse0, pulse1, data_len);

            break;


        default:
            ESP_LOGE(TAG,"ERROR STATE\n");
            //exit(-1);
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
            tzx_chunk(&t, buf,sizeof(buf));
    } while (r>0);
    close(fd);

}
#endif
