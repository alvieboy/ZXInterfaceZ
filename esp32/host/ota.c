#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

const esp_partition_t *esp_ota_get_boot_partition(void)
{
    const esp_partition_t * r = NULL;
    esp_partition_iterator_t i = esp_partition_find(ESP_PARTITION_TYPE_APP,  ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (i) {
        r = esp_partition_get(i);
    }
    esp_partition_iterator_release(i);
    return r;
}

const esp_partition_t *esp_ota_get_running_partition(void)
{
    const esp_partition_t * r = NULL;
    esp_partition_iterator_t i = esp_partition_find(ESP_PARTITION_TYPE_APP,  ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (i) {
        r = esp_partition_get(i);
    }
    esp_partition_iterator_release(i);
    return r;
}

const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start_from __attribute__((unused)))
{
    const esp_partition_t * r = NULL;
    esp_partition_iterator_t i = esp_partition_find(ESP_PARTITION_TYPE_APP,  ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (i) {
        r = esp_partition_get(i);
    }
    esp_partition_iterator_release(i);
    return r;
}

static struct {
    const esp_partition_t *p;
    size_t size;
    unsigned off;
} ota_partitions[1];

esp_err_t esp_ota_begin(const esp_partition_t *partition, size_t image_size, esp_ota_handle_t *out_handle)
{
    *out_handle = 0;
    ota_partitions[0].p  = partition;
    ota_partitions[0].size = image_size;
    ota_partitions[0].off = 0;
    return ESP_OK;
}

esp_err_t esp_ota_write(esp_ota_handle_t handle, const void *data, size_t size)
{
    int r = esp_partition_write(ota_partitions[handle].p,
                                ota_partitions[handle].off,
                                data, size);
    if (r<0)
        return r;

    ota_partitions[handle].off += size;

    return r;
}

esp_err_t esp_ota_end(esp_ota_handle_t handle)
{
    return ESP_OK;
}

esp_err_t esp_ota_set_boot_partition(const esp_partition_t *partition)
{
    return ESP_OK;
}
