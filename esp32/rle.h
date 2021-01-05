#define MIN_RUN     5                   /* minimum run length to encode */
#define MAX_RUN     (32767 + MIN_RUN - 1) /* maximum run length to encode */
#define MAX_COPY    32767                 /* maximum characters to copy */
#define MAX_READ    (MAX_COPY + MIN_RUN - 1)


struct rle_decoder_buf {
    enum {
        READ_SIZE_H,
        READ_SIZE_L,
        READ_BUF
    } state;
    union {
        uint16_t size;
        uint8_t sizebuf[2];
    };
    int (*writer)(void *, const uint8_t *data, size_t len);
    void *userdata;
};
