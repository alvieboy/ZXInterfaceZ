#include "tap.h"
#include "byteops.h"


#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define TAP_STANDARD_GAP 1000

static int tap__check(struct tap *t, const uint8_t **data, int *len, int needed)
{
    while (t->tapbufptr<needed && (*len>0) ) {
        t->tapbuf[t->tapbufptr++] = *(*data);
        (*len)--;
        (*data)++;
    }
    if (t->tapbufptr==needed) {
        t->tapbufptr = 0;
        return 0;
    }
    return -1;
}

void tap__init(struct tap *t)
{
    t->state = LENGTH;
    t->tapbufptr = 0;
}

void tap__chunk(struct tap *t, const uint8_t *data, int len)
{
    int alen;

#define NEED(x) if (tap__check(t, &data, &len, x)<0) return;
    if (len==0) {
        tap__finished_callback();
        return;
    }
    //ESP_LOGI(TAG, "Parse chunk len %d", len);
    do {
        switch (t->state) {
        case LENGTH:
            NEED(2);
            t->datachunk = extractle16(&t->tapbuf[0]);
            tap__standard_block_callback( t->datachunk , TAP_STANDARD_GAP );
            t->state = DATA;
            break;
        case DATA:
            alen = MIN((int)len, (int)t->datachunk);

            tap__data_callback(data, alen);

            t->datachunk-=alen;
            len-=alen;
            data+=alen;

            if (t->datachunk==0) {
                tap__data_finished_callback();
                t->state = LENGTH;
            }

            break;
        }
    } while (len>0);
}
