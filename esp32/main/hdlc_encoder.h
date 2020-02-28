#ifndef __HDLC_ENCODER_H__
#define __HDLC_ENCODER_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t crc;
    void (*writer)(void *userdata, const uint8_t c);
    void (*flusher)(void *userdata);
    void *userdata;
} hdlc_encoder_t;

uint8_t* hdlc_encoder__begin_mem(hdlc_encoder_t *hdlc, uint8_t *target);
uint8_t *hdlc_encoder__write_mem(hdlc_encoder_t *hdlc, const uint8_t *data, uint16_t datalen, uint8_t *target);
uint8_t *hdlc_encoder__write_mem_byte(hdlc_encoder_t *hdlc, uint8_t data, uint8_t *target);
uint8_t *hdlc_encoder__end_mem(hdlc_encoder_t *hdlc, uint8_t *target);

int hdlc_encoder__init(hdlc_encoder_t *hdlc, void (*writer)(void*,const uint8_t), void (*flusher)(void*), void*);
int hdlc_encoder__begin(hdlc_encoder_t *hdlc);
int hdlc_encoder__write(hdlc_encoder_t *hdlc, const void *data, uint16_t datalen);
int hdlc_encoder__end(hdlc_encoder_t *hdlc);

#define HDLC_MAX_ENCODE_SIZE(x) (1+((x)*2)+2+2+1)

#ifdef __cplusplus
}
#endif

#endif
