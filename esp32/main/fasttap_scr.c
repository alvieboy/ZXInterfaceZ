#include "fasttap_scr.h"
#include "log.h"
#include <unistd.h>
#include "esp_attr.h"
#include "fpga.h"


#define TAG "TAG-SCR"
// Keep this in RAM to use DMA for SPI transfer

DMA_ATTR uint8_t scr_tap_header[16] = { // No checksum, hence 17-1
    0x03,0x70,0x20,0x20,
    0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x00,
    0x1b,0x00,0x40
};

struct fasttap_scr {
    fasttap_t fasttap;
    uint8_t scr_chunk;
};

static void fasttap_scr__free(fasttap_t *fasttap);
static int fasttap_scr__next(fasttap_t *fasttap, uint8_t type, uint16_t len);
static int fasttap_scr__init(fasttap_t *fasttap);
static void fasttap_scr__stop(fasttap_t *fasttap);

static struct fasttap_ops fasttap_scr_ops = {
    .next = &fasttap_scr__next,
    .init = &fasttap_scr__init,
    .stop = &fasttap_scr__stop,
    .free = &fasttap_scr__free,
    .finished = &fasttap__is_file_eof
};

/**
 * \ingroup fasttap
 * \brief Allocate a new SCR Fast TAP
 * \return The newly allocated Fast TAP, or NULL if some error ocurred
 */
fasttap_t *fasttap_scr__allocate(void)
{
    struct fasttap_scr *self = (struct fasttap_scr*)malloc(sizeof(struct fasttap_scr));
    if (NULL==self)
        return NULL;

    self->fasttap.ops = &fasttap_scr_ops;

    return &self->fasttap;
}

static void fasttap_scr__free(fasttap_t *fasttap)
{
    free(fasttap);
}

static int fasttap_scr__init(fasttap_t *fasttap)
{
    struct fasttap_scr *self = (struct fasttap_scr*)fasttap;
    self->scr_chunk = 0;

    if (fasttap__install_hooks( model__get() )<0)
        return -1;

    return 0;
}

static int fasttap_scr__next(fasttap_t *fasttap, uint8_t requested_type, uint16_t requested_len)
{
    struct fasttap_scr *self = (struct fasttap_scr*)fasttap;
    uint16_t len;
    int r = 0;

    ESP_LOGI(TAG,"SCR request next block, chunk %d", self->scr_chunk);
    switch (self->scr_chunk) {
    case 0:
        /* Header */
        len = 16;
        r = fpga__write_extram_block(FASTTAP_ADDRESS_DATA, scr_tap_header, len);
        fpga__write_extram_block(FASTTAP_ADDRESS_LENLSB, (uint8_t*)&len, 2);
        self->scr_chunk++;
        break;
    case 1:
        /* Data */
        len = 6912;
        r = fpga__write_extram_block_from_file(FASTTAP_ADDRESS_DATA, fasttap->fd, len);
        fpga__write_extram_block(FASTTAP_ADDRESS_LENLSB, (uint8_t*)&len, 2);
        self->scr_chunk++;
        fasttap__stop();
        break;
    default:
        r = -1;
        break;

    }
    return r;
}

static void fasttap_scr__stop(fasttap_t *fasttap)
{
}
