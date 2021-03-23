/**
 \ingroup tools
 \defgroup bitmap_allocator Bitmap allocator
 \brief Bitmap (32-bit) allocator

 Provides a safe bitmap allocator with 32 entries.

*/

#include "bitmap_allocator.h"
#include "log.h"
#include "esp_assert.h"

static void bitmap_allocator__lock(bitmap32_t *bitmap)
{
    if (xSemaphoreTake( bitmap->sem,  portMAX_DELAY )!= pdTRUE) {
        ESP_LOGE("BITMAP", "Cannot take semaphore");
        assert(false);
    }
}
static void bitmap_allocator__unlock(bitmap32_t *bitmap)
{
    xSemaphoreGive(bitmap->sem);
}
/**
 \ingroup bitmap_allocator
 \brief Initialize a new bitmap

 Initialize a new bitmap.

 \param bitmap Pointer to the bitmap to be initialised.
 */
void bitmap_allocator__init(bitmap32_t *bitmap)
{
    bitmap->sem = xSemaphoreCreateMutex();
    bitmap->bitmap = 0;
}

/**
 \ingroup bitmap_allocator
 \brief Allocate a new entry on the bitmap

 \param bitmap Pointer to the bitmap to be used
 \return -1 if no free entries are available, otherwise returns the bitmap index.

 */
int bitmap_allocator__alloc(bitmap32_t *bitmap)
{
    bitmap_allocator__lock(bitmap);
    uint32_t mask = 0x01;
    int index = 0 ;

    do {
        if (mask & bitmap->bitmap) {
            index++;
            mask<<=1;
            continue;
        }
        bitmap->bitmap |= mask;
        break;
    } while (mask);

    if (index>31)
        index = -1;
    bitmap_allocator__unlock(bitmap);
    return index;
}

/**
 \ingroup bitmap_allocator
 \brief Release a bitmap entry

 The bitmap index should be between 0 and 31. If the index is already released, this function
 does nothing.

 \param bitmap Pointer to the bitmap to be used
 \param index Index to be released.

 */
void bitmap_allocator__release(bitmap32_t *bitmap, int index)
{
    uint32_t mask = 0x1 << index;
    bitmap_allocator__lock(bitmap);
    bitmap->bitmap &= ~mask;
    bitmap_allocator__unlock(bitmap);
}
