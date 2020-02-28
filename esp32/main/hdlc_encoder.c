#include "hdlc_encoder.h"
#include <stdlib.h>

#define HDLC_FRAME_FLAG 0x7E
#define HDLC_ESCAPE_FLAG 0x7D
#define HDLC_ESCAPE_XOR 0x20

static void hdlc_encoder__update_crc(hdlc_encoder_t *hdlc, uint8_t data)
{
    data ^= hdlc->crc&0xff;
    data ^= data << 4;
    hdlc->crc = ((((uint16_t)data << 8) | ((hdlc->crc>>8)&0xff)) ^ (uint8_t)(data >> 4)
                 ^ ((uint16_t)data << 3));
}


int hdlc_encoder__init(hdlc_encoder_t *hdlc, void (*writer)(void*,const uint8_t c), void (*flusher)(void*), void*userdata)
{
    hdlc->writer = writer;
    hdlc->flusher = flusher;
    hdlc->userdata = userdata;
    return 0;
}

int hdlc_encoder__begin(hdlc_encoder_t *hdlc)
{
    hdlc->crc = 0xFFFF;
    hdlc->writer(hdlc->userdata, HDLC_FRAME_FLAG);
    return 1;
}

int hdlc_encoder__write(hdlc_encoder_t *hdlc, const void *udata, uint16_t datalen)
{
    int c = 0;
    const uint8_t *data = (const uint8_t*)udata;
    while (datalen--) {
        hdlc_encoder__update_crc(hdlc, *data);
        if ((*data==HDLC_FRAME_FLAG) || (*data==HDLC_ESCAPE_FLAG)) {
            hdlc->writer(hdlc->userdata, HDLC_ESCAPE_FLAG);
            hdlc->writer(hdlc->userdata, *data ^ HDLC_ESCAPE_XOR);
            c+=2;
        } else {
            hdlc->writer(hdlc->userdata, *data);
            c++;
        }
        data++;
    }
    return c;
}

int hdlc_encoder__end(hdlc_encoder_t *hdlc)
{
    int r;
    uint8_t c[2];

    c[0] = hdlc->crc & 0xff;
    c[1] = hdlc->crc >> 8;

    r = hdlc_encoder__write(hdlc, c, sizeof(c));
    hdlc->writer(hdlc->userdata, HDLC_FRAME_FLAG);
    if (hdlc->flusher!=NULL)
        hdlc->flusher(hdlc->userdata);
    r++;
    return r;
}






uint8_t* hdlc_encoder__begin_mem(hdlc_encoder_t *hdlc, uint8_t *target)
{
    hdlc->crc = 0xFFFF;
    *target++ = HDLC_FRAME_FLAG;
    return target;
}

uint8_t *hdlc_encoder__write_mem(hdlc_encoder_t *hdlc, const uint8_t *data, uint16_t datalen, uint8_t *target)
{
    while (datalen--) {
        hdlc_encoder__update_crc(hdlc, *data);
        if ((*data==HDLC_FRAME_FLAG) || (*data==HDLC_ESCAPE_FLAG)) {
            *target++ = HDLC_ESCAPE_FLAG;
            *target++ = *data ^ HDLC_ESCAPE_XOR;
        } else {
            *target++ = *data;
        }
        data++;
    }
    return target;
}

uint8_t *hdlc_encoder__write_mem_byte(hdlc_encoder_t *hdlc, const uint8_t data, uint8_t *target)
{
    hdlc_encoder__update_crc(hdlc, data);
    if ((data==HDLC_FRAME_FLAG) || (data==HDLC_ESCAPE_FLAG)) {
        *target++ = HDLC_ESCAPE_FLAG;
        *target++ = data ^ HDLC_ESCAPE_XOR;
    } else {
        *target++ = data;
    }
    return target;
}

uint8_t *hdlc_encoder__end_mem(hdlc_encoder_t *hdlc, uint8_t *target)
{
    uint8_t c[2];

    c[0] = hdlc->crc & 0xff;
    c[1] = hdlc->crc >> 8;

    target = hdlc_encoder__write_mem(hdlc, c, sizeof(c), target);
    *target++=HDLC_FRAME_FLAG;
    return target;
}

