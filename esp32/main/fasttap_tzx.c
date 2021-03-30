#include "fasttap_tzx.h"
#include "tzx.h"
#include <stdlib.h>
#include <fpga.h>
#include "log.h"
#include <unistd.h>

/* TZX version */

#define TAG "FASTTAP-TZX"

struct fasttap_tzx {
    fasttap_t fasttap;
    struct tzx *tzx;
    int tzx_error;
    int tzx_block_done;
    uint16_t expected_size;
    uint16_t tzx_offset;
    uint16_t tzx_block_size;
};

static void fasttap_tzx__free(fasttap_t *fasttap);
static int fasttap_tzx__next(fasttap_t *fasttap, uint8_t type, uint16_t len);
static int fasttap_tzx__init(fasttap_t *fasttap);
static void fasttap_tzx__stop(fasttap_t *fasttap);

static struct fasttap_ops fasttap_tzx_ops = {
    .next = &fasttap_tzx__next,
    .init = &fasttap_tzx__init,
    .stop = &fasttap_tzx__stop,
    .free = &fasttap_tzx__free,
    .finished = &fasttap__is_file_eof
};

/**
 * \ingroup fasttap
 * \brief Allocate a new TZX Fast TAP
 * \return The newly allocated Fast TAP, or NULL if some error ocurred
 */
fasttap_t *fasttap_tzx__allocate(void)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)malloc(sizeof(struct fasttap_tzx));
    if (NULL==self)
        return NULL;

    self->fasttap.ops = &fasttap_tzx_ops;

    return &self->fasttap;
}

static void fasttap_tzx__free(fasttap_t *fasttap)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)fasttap;
    if (self->tzx) {
        free(self->tzx);
    }
    free(self);
}


static void fasttap__tzx_standard_block_callback(void *userdata, uint16_t length, uint16_t pause_after)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)userdata;
    self->expected_size = length;
    self->tzx_block_size= length;
    self->tzx_offset = 0;
}

static void fasttap__tzx_turbo_block_callback(void *userdata,
                                              uint16_t pilot,
                                              uint16_t sync0,
                                              uint16_t sync1,
                                              uint16_t pulse0,
                                              uint16_t pulse1,
                                              uint16_t pilot_len,
                                              uint16_t gap_len,
                                              uint32_t data_len,
                                              uint8_t last_byte_len)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)userdata;
    self->tzx_error = 1;
}

static void fasttap__tzx_pure_data_callback(void*userdata,uint16_t pulse0, uint16_t pulse1, uint32_t data_len, uint16_t gap,
                                            uint8_t last_byte_len)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)userdata;
    self->tzx_error = 1;
}

static void fasttap__tzx_data_callback(void*userdata, const uint8_t *data, int len)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)userdata;

    if (len<=self->expected_size) {
        // We need to skip type and final checksum.
        if (self->tzx_offset==0) {
            self->tzx_offset++;
            self->expected_size--;
            len--;
            data++;
        }
        if (len>0) {
            //ESP_LOGI(TAG, "Emmiting %d bytes at offset %d", len, tzx_offset-1);
            //BUFFER_LOGI(TAG, "data", data, len);
            fpga__write_extram_block(FASTTAP_ADDRESS_DATA + (unsigned)self->tzx_offset-1, data, len);
            self->expected_size -= len;
            self->tzx_offset+=len;
        }
    } else {
        ESP_LOGE(TAG,"Data too big for block!");
    }

}

void fasttap__tzx_tone_callback(void *userdata,uint16_t t_states, uint16_t count)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)userdata;
    self->tzx_error = 1;
}

void fasttap__tzx_pulse_callback(void *userdata,uint8_t count, const uint16_t *t_states /* these are words */)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)userdata;
    self->tzx_error = 1;
}

void fasttap__tzx_data_finished_callback(void *userdata)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)userdata;
    ESP_LOGI(TAG,"Data finished");
    // expected_size should be zero
    if(self->expected_size>0)
        ESP_LOGE(TAG,"Expected size greater than actual size!");

    self->tzx_block_size -= 2; // Remove leading type and checksum

    fpga__write_extram_block(FASTTAP_ADDRESS_LENLSB, (uint8_t*)&self->tzx_block_size, 2);

    self->tzx_block_done = 1;
    self->tzx_offset = 0;
}

void fasttap__tzx_finished_callback(void*userdata)
{
    fasttap__stop();
}

const struct tzx_callbacks fasttap_tzx_callbacks =
{
    .standard_block_callback = fasttap__tzx_standard_block_callback,
    .turbo_block_callback    = fasttap__tzx_turbo_block_callback,
    .data_callback           = fasttap__tzx_data_callback,
    .tone_callback           = fasttap__tzx_tone_callback,
    .pulse_callback          = fasttap__tzx_pulse_callback,
    .pure_data_callback      = fasttap__tzx_pure_data_callback,
    .data_finished_callback  = fasttap__tzx_data_finished_callback,
    .finished_callback       = fasttap__tzx_finished_callback
};



static int fasttap_tzx__init(fasttap_t *fasttap)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)fasttap;

    int r = tzx__can_fastplay_fd(fasttap->fd);
    if (r<0) {
        return r;
    }
    if (r!=0) {
        return -1;
    }
    lseek(fasttap->fd,0,SEEK_SET);

    self->tzx = malloc(sizeof(struct tzx));
    if (self->tzx==NULL) {
        return -1;
    }

    tzx__init(self->tzx,&fasttap_tzx_callbacks, self);

    if (fasttap__install_hooks( model__get() )<0)
        return -1;

    ESP_LOGI(TAG,"Tape ready to fast load (TZX)");
    return 0;
}

static int fasttap_tzx__next(fasttap_t *fasttap, uint8_t type, uint16_t len)
{
    struct fasttap_tzx *self = (struct fasttap_tzx*)fasttap;
    uint8_t buf[4]; // This *MUST* be smaller than the minimum TZX block size
    int r;

    self->tzx_block_done = 0;
    ESP_LOGI(TAG,"TZX fast load next block");
    do {
        r = read(fasttap->fd, buf, sizeof(buf));
        if (r<0) {
            ESP_LOGI(TAG,"TZX error read!");
            return -1;
        }
        //ESP_LOGI(TAG,"TZX fast load parse chunk");
        tzx__chunk(self->tzx, buf, r);
    } while (!self->tzx_block_done);

    ESP_LOGI(TAG,"TZX fast load block done");
    return 0;
}

static void fasttap_tzx__stop(fasttap_t *fasttap)
{
}
