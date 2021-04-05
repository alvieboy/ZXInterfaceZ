#include <inttypes.h>
#include <sys/types.h>
#include "rle.h"
#include "esp_log.h"
#include "minmax.h"
#include "stream.h"

#define TAG "RLE"

#define RLE_READ_BLOCK_SIZE 128

static int rle_read_s16(int (*reader)(void*user, uint8_t*buf,size_t), void *read_user, int *target)
{
    union {
        int16_t size;
        uint8_t sizebuf[2];
    } v;
    int r = reader(read_user, &v.sizebuf[0], sizeof(int16_t));

    if (r!=sizeof(int16_t)) {
        ESP_LOGE(TAG,"Cannot read run block size!");
        return -1;
    }

    (*target) = (int)v.size;

    return 0;
}

static int rle_copy_block(int size, int (*reader)(void*user, uint8_t*buf,size_t), void *read_user,
                          int (*writer)(void*user, const uint8_t*buf,size_t), void *write_user)
{
    uint8_t buffer[RLE_READ_BLOCK_SIZE];
    int chunk;
    int r;

    while (size>0) {
        chunk = MIN(size,(int)sizeof(buffer));
        //ESP_LOGI(TAG,"Read chunk %d", chunk);
        r = reader(read_user, buffer, chunk);
        if (r!=chunk) {
            ESP_LOGE(TAG,"copy block is too short!");
            return -1;
        }
        r = writer(write_user, buffer, chunk);
        if (r<0) {
            ESP_LOGE(TAG,"cannot copy block!");
            return -1;
        }
        size -= chunk;
    }
    return 0;
}

int rle_decompress_stream_fn(int (*reader)(void*user, uint8_t*buf,size_t), void *read_user,
                          int (*writer)(void*user, const uint8_t*buf,size_t), void *write_user,
                          int sourcelen)
{
    int countChar;
    uint8_t currChar;
    int r;
    uint8_t repblock[RLE_READ_BLOCK_SIZE];

    while (sourcelen>1) {
        r = rle_read_s16(reader, read_user, &countChar);

        if (r<0)
            return -1;

        sourcelen -= 2;

        if (countChar < 0)
        {
            /* we have a run write out  2 - countChar copies */
            countChar = (MIN_RUN - 1) - countChar;

            r = reader(read_user, &currChar, sizeof(uint8_t));

            if (r<=0)
            {
                ESP_LOGE(TAG, "run block too short!");
                return -1;
                countChar = 0;
            }
            //ESP_LOGI(TAG, "Run block 0x%02x : %d", currChar, countChar);

            memset( repblock, currChar, sizeof(repblock));
            while (countChar > 0)
            {
                int chunk = MIN(countChar, RLE_READ_BLOCK_SIZE);

                if (writer(write_user, repblock, chunk)<0)  {
                    ESP_LOGE(TAG, "Error writing run block");
                    return -1;
                }
                countChar-=chunk;
            }
            sourcelen--;

        }
        else
        {
            countChar++;
            //printf("Block %d (%d bytes)\n", countChar, countChar+1);
            //ESP_LOGI(TAG, "Copy block %d", countChar);
            if (rle_copy_block(countChar, reader, read_user, writer, write_user)<0) {
                ESP_LOGE(TAG, "Cannot copy run block");

                return -1;
            }

            sourcelen -= countChar;
        }
    }
    return 0;
}

int rle_decompress_stream(struct stream * s,
                          int (*writer)(void*user, const uint8_t*buf,size_t), void *write_user,
                          int sourcelen)
{
    return rle_decompress_stream_fn( (int(*)(void*,uint8_t*,size_t))&stream__read, s,
                                    writer, write_user, sourcelen);
}
