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

void bitmap_allocator__init(bitmap32_t *bitmap)
{
    bitmap->sem = xSemaphoreCreateMutex();
    bitmap->bitmap = 0;
}

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

void bitmap_allocator__release(bitmap32_t *bitmap, int index)
{
    uint32_t mask = 0x1 << index;
    bitmap_allocator__lock(bitmap);
    bitmap->bitmap &= ~mask;
    bitmap_allocator__unlock(bitmap);
}
