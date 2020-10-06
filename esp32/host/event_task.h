#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

struct esp_event_wrap
{
    esp_event_base_t event_base;
    int32_t event_id;
    void* event_data;
    size_t event_data_size;
};

void event_task(void *);
void event_task_init(void);

#ifdef __cplusplus
}
#endif
