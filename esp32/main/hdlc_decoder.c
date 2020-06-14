#include "hdlc_decoder.h"
#include <stddef.h>

#define HDLC_FRAME_FLAG 0x7E
#define HDLC_ESCAPE_FLAG 0x7D
#define HDLC_ESCAPE_XOR 0x20

static void hdlc_decoder__update_crc(hdlc_decoder_t *hdlc, uint8_t data)
{
    data ^= hdlc->crc&0xff;
    data ^= data << 4;
    hdlc->crc = ((((uint16_t)data << 8) | ((hdlc->crc>>8)&0xff)) ^ (uint8_t)(data >> 4)
                 ^ ((uint16_t)data << 3));
}

void hdlc_decoder__init(hdlc_decoder_t *decoder, uint8_t *buffer, unsigned maxlen,
                        hdlc_handler_t handler, void *handlerdata)
{
    decoder->escape = false;
    decoder->sync = false;
    decoder->buffer = buffer;
    decoder->buflen = maxlen;
    decoder->handler = handler;
    decoder->handlerdata = handlerdata;
    decoder->clen = 0;
}

static void hdlc_decoder__process_frame(hdlc_decoder_t *decoder)
{
    if (decoder->sync && decoder->clen>=2 && (decoder->crc==0)) {
        decoder->handler(decoder->handlerdata,
                         decoder->buffer,
                         decoder->clen - 2);
    } else {
        decoder->handler(decoder->handlerdata,
                         NULL,
                         0
                        );
    }

}

void hdlc_decoder__append(hdlc_decoder_t *decoder, uint8_t data)
{
    if (decoder->sync) {
        if (data==HDLC_FRAME_FLAG) {
            if (decoder->clen>0) {
                hdlc_decoder__process_frame(decoder);
                decoder->sync = false;
            }
        } else if (data==HDLC_ESCAPE_FLAG) {
            decoder->escape = true;
        } else if (decoder->clen < decoder->buflen ) {
            if (decoder->escape) {
                decoder->escape = false;
                data^=HDLC_ESCAPE_XOR;
            }
            hdlc_decoder__update_crc(decoder, data);
            decoder->buffer[decoder->clen++]=data;
        } else {
            // Frame too big.
            decoder->sync = false;
            //debug__printf("HDLC: frame too big, dropping\r\n");
            hdlc_decoder__process_frame(decoder);
        }
    } else {
        if (data==HDLC_FRAME_FLAG) {
            decoder->clen=0;
            decoder->crc = 0xFFFF;
            decoder->sync=true;
            decoder->escape=false;
        } 
    }
}

void hdlc_decoder__append_buffer(hdlc_decoder_t *decoder, const uint8_t *data, unsigned datalen)
{
    while (datalen--) {
        hdlc_decoder__append(decoder, *data++);
    }
}
