#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "defs.h"
#include "resource.h"
#include "flash_resource.h"
#include "fpga.h"

static const esp_partition_t *resource_partition = NULL;
static const uint8_t *resource_ptr = NULL;
static spi_flash_mmap_handle_t mmap_handle;

void flash_resource__init(void)
{
    esp_partition_iterator_t i =
        esp_partition_find(0x41, 0x00, NULL);
    if (i!=NULL) {
        resource_partition = esp_partition_get(i);

        esp_err_t err =esp_partition_mmap(resource_partition,
                                          0, /* Offset */
                                          resource_partition->size,
                                          SPI_FLASH_MMAP_DATA,
                                          (const void**)&resource_ptr,
                                          &mmap_handle);
        ESP_ERROR_CHECK(err);
        ESP_LOGI(TAG,"Mapped resource partition at %p", resource_ptr);
    } else {
        ESP_LOGW(TAG,"Cannot find resource partition!!!");

    }
    esp_partition_iterator_release(i);
}

static const struct flashresourcedata *flashresourcedata__finddata(uint8_t id)
{
    if (resource_ptr==NULL)
        return NULL;

    uint16_t num_resources = *(uint16_t*)resource_ptr;
    const uint8_t *rptr = &resource_ptr[2];

    ESP_LOGI(TAG,"Requested resource ID %d, total resources %d", id, (int)num_resources);
    for (uint16_t r = 0; r<num_resources; r++) {
        uint8_t resid = *rptr++;
        const struct flashresourcedata *res = (const struct flashresourcedata*) rptr;

        if (resid==id) {
            return res;
        } else {
            rptr += 2; // Move past type and resource size.
            rptr += res->len;
        }
    }
    return NULL;
}


uint8_t flash_resource__type(struct resource *r)
{
    struct flash_resource *fr = (struct flash_resource*)r;
    return fr->data->type;
}

uint16_t flash_resource__len(struct resource *r)
{
    struct flash_resource *fr = (struct flash_resource*)r;
    ESP_LOGI(TAG,"Flash resource size: %d", fr->data->len);
    return fr->data->len;
}

int flash_resource__sendToFifo(struct resource *r)
{
    struct flash_resource *fr = (struct flash_resource*)r;
    return fpga__load_resource_fifo( fr->data->data, fr->data->len, RESOURCE_DEFAULT_TIMEOUT);
}

static struct flash_resource flashresource = {
    .r = {
        .type = &flash_resource__type,
        .len = &flash_resource__len,
        .sendToFifo = &flash_resource__sendToFifo,
        .update = NULL,
    },
    .data = NULL
};

struct flash_resource *flash_resource__find(uint8_t id)
{
    const struct flashresourcedata *data = flashresourcedata__finddata(id);
    if (data==NULL) {
        return NULL;
    }
    flashresource.data = data;
    return &flashresource;
}
