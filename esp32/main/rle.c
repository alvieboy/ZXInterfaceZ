#include <inttypes.h>
#include <sys/types.h>
#include "rle.h"
#include "esp_log.h"

#define TAG "RLE"

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
    uint8_t buffer[32];
    int chunk;
    int r;

    while (size>0) {
        chunk = size > sizeof(buffer)? sizeof(buffer):size;
        ESP_LOGI(TAG,"Read chunk %d", chunk);
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

int rle_decompress_stream(int (*reader)(void*user, uint8_t*buf,size_t), void *read_user,
                          int (*writer)(void*user, const uint8_t*buf,size_t), void *write_user,
                          int sourcelen)
{
    int countChar;
    uint8_t currChar;
    int r;

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
            ESP_LOGI(TAG, "Run block 0x%02x : %d", currChar, countChar);

            while (countChar > 0)
            {
                if (writer(write_user, &currChar,sizeof(uint8_t))<0)  {
                    ESP_LOGE(TAG, "Error writing run block");
                    return -1;
                }
                countChar--;
            }
            sourcelen--;

        }
        else
        {
            countChar++;
            //printf("Block %d (%d bytes)\n", countChar, countChar+1);
            ESP_LOGI(TAG, "Copy block %d", countChar);
            if (rle_copy_block(countChar, reader, read_user, writer, write_user)<0) {
                ESP_LOGE(TAG, "Cannot copy run block");

                return -1;
            }

            sourcelen -= countChar;
        }
    }
    return 0;
}
