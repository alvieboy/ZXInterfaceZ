#include "fasttap.h"
#include "fasttap_tap.h"
#include "log.h"
#include "fpga.h"
#include <unistd.h>

#define TAG "FASTTAP-TAP"

static void fasttap_tap__free(fasttap_t *fasttap);
static int fasttap_tap__next(fasttap_t *fasttap, uint8_t type, uint16_t len);
static int fasttap_tap__init(fasttap_t *fasttap);
static void fasttap_tap__stop(fasttap_t *fasttap);

static struct fasttap_ops fasttap_tap_ops = {
    .next = &fasttap_tap__next,
    .init = &fasttap_tap__init,
    .stop = &fasttap_tap__stop,
    .free = &fasttap_tap__free,
    .finished = &fasttap__is_file_eof
};

/**
 * \ingroup fasttap
 * \brief Allocate a new TAP Fast TAP
 * \return The newly allocated Fast TAP, or NULL if some error ocurred
 */
fasttap_t *fasttap_tap__allocate(void)
{
    fasttap_t *self = (fasttap_t*)malloc(sizeof(fasttap_t));
    if (NULL==self)
        return NULL;

    self->ops = &fasttap_tap_ops;

    return self;
}

static void fasttap_tap__free(fasttap_t *fasttap)
{
    free(fasttap);
}

static int fasttap_tap__init(fasttap_t *fasttap)
{
    if (fasttap__install_hooks( model__get() )<0)
        return -1;

    ESP_LOGI(TAG,"Tape ready to fast load");
    return 0;
}

static int fasttap_tap__next(fasttap_t *fasttap, uint8_t requested_type, uint16_t requested_len)
{
    uint16_t len;
    uint8_t type;
    if (fasttap->stream==NULL) {
        ESP_LOGE(TAG,"Invalid stream descriptor!");
        return -1;
    }

    if (fasttap__read(fasttap, &len, 2)<2) {
        ESP_LOGE(TAG,"Short read from stream");
        return -1;
    }

    ESP_LOGI(TAG, "Block len %d", len);

    if (fasttap__read(fasttap, &type, 1)<1)
        return -1;

    ESP_LOGI(TAG, "Block type %d", type);

    // Assert block size.

    len -= 1; // Skip type

    int r = fpga__write_extram_block_from_stream(FASTTAP_ADDRESS_DATA, fasttap->stream, len);

    fasttap->read += len;

    if (r<0) {
        ESP_LOGE(TAG, "Cannot read TAP file");
        //close(fasttap_fd);
        //fasttap_fd = -1;
        return -1;
    }
    len-=1; // Skip checksum copy

    fpga__write_extram_block(FASTTAP_ADDRESS_LENLSB, (uint8_t*)&len, 2);
    // Mark ready
    ESP_LOGI(TAG,"Block ready (%d)",len);

    if (fasttap__is_file_eof(fasttap)) {
        ESP_LOGI(TAG, "Reached EOF");
        fasttap__stop();
    }

    return 0;
}

static void fasttap_tap__stop(fasttap_t *fasttap)
{
}
