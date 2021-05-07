#include "tap.h"
#include "byteops.h"
#include "esp_log.h"
#include "minmax.h"
#include "byteorder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "TAP"


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
    t->initial_delay = 0;
}

void tap__set_initial_delay(struct tap *t, int initial_delay)
{
    t->initial_delay = initial_delay;
}

void tap__chunk(struct tap *t, const uint8_t *data, int len)
{
    int alen;

#define NEED(x) if (tap__check(t, &data, &len, x)<0) return;
    if (len==0) {
        tap__finished_callback();
        return;
    }
    //ESP_LOGI(TAG, "Parse chunk len %d state %d", len, t->state);

    if (t->initial_delay) {
        vTaskDelay((int)t->initial_delay/portTICK_RATE_MS);
        t->initial_delay = 0;
    }

    do {
        switch (t->state) {
        case LENGTH:
            NEED(2);
            t->datachunk = extractle16(&t->tapbuf[0]);
            ESP_LOGI(TAG, "Standard block ahead, %d bytes", t->datachunk );
            tap__standard_block_callback( t->datachunk , TAP_STANDARD_GAP );
            t->state = DATA;
            break;
        case DATA:
            alen = MIN((int)len, (int)t->datachunk);

            //ESP_LOGI(TAG, "Data %d bytes", alen);

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
