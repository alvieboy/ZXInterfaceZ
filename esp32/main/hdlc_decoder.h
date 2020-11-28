#ifndef __HDLC_DECODER_H__
#define __HDLC_DECODER_H__

#include <stdbool.h>
#include <inttypes.h>

typedef void (*hdlc_frame_handler_t)(void*, const uint8_t *data, unsigned datalen);
typedef void (*hdlc_invalid_char_handler_t)(void*, uint8_t data);

typedef struct {
    uint8_t *buffer;
    unsigned buflen;
    unsigned clen;
    hdlc_frame_handler_t frame_handler;
    hdlc_invalid_char_handler_t invalid_handler;
    void *handlerdata;
    uint16_t crc;
    bool escape;
    bool sync;
} hdlc_decoder_t;


void hdlc_decoder__init(hdlc_decoder_t *decoder, uint8_t *buffer, unsigned maxlen,
                        hdlc_frame_handler_t handler,
                        hdlc_invalid_char_handler_t invalid_handler,
                        void *handlerdata);
void hdlc_decoder__append(hdlc_decoder_t *decoder, uint8_t data);
void hdlc_decoder__append_buffer(hdlc_decoder_t *decoder, const uint8_t *data, unsigned datalen);
void hdlc_decoder__reset(hdlc_decoder_t *decoder);

#endif
