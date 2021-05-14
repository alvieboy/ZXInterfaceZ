#include "firmware_ws.h"
#include "log.h"
#include "os/task.h"
#include "os/queue.h"
#include "os/core.h"
#include "stream.h"
#include "firmware.h"
#include "interfacez_tasks.h"
#include "defs.h"

#undef TAG
#define TAG "FIRMWAREWS"

struct wsdata {
    unsigned len;
    uint8_t *data;
};

static const progress_reporter_t ws_reporter;

static Queue firmwarews_data_queue = NULL;

void firmware_ws__task(void *data)
{
    httpd_req_t *req = (httpd_req_t*)data;

    firmware_upgrade_t f;
    struct stream *stream = stream__alloc_websocket(req, NULL); // Fix finish callback

    firmware__init(&f, stream);

    stream__nonblock(stream, false);  // Force block

    // Initialise reporter
    firmware__set_reporter(&f, &ws_reporter, req);

    ESP_LOGI(TAG,"Starting FW upgrade via websocket, req %p queue %p", req, req->sess_ctx);

    int r = firmware__upgrade(&f);

    firmware__deinit(&f);

    stream__destroy(stream);

    // destroy queue. TBD proper order..
    queue__delete(firmwarews_data_queue);

    firmwarews_data_queue = NULL;

    if (r==0)
        request_restart();


    task__delete(NULL);
}

void firmware_ws__free_ctx(void*queue)
{
    // TBD...
}

int firmware_ws__start_process(httpd_req_t *req)
{
    if (firmwarews_data_queue!=NULL) {
        ESP_LOGE(TAG, "Re-entry of firmware is not possible!");
        return -1;
    }

    firmwarews_data_queue  = queue__create(2, sizeof(struct wsdata));

    req->sess_ctx = firmwarews_data_queue;
    req->free_ctx = &firmware_ws__free_ctx;

    task__create(firmware_ws__task, "firmware_task", FIRMWAREWS_TASK_STACK_SIZE, req, FIRMWAREWS_TASK_PRIORITY, NULL);

    return 0;
}

int firmware_ws__read(httpd_req_t *req, void *buf, size_t size, bool block)
{
    struct wsdata data;
    size_t bytes_read = 0;


    Queue data_queue = (Queue)req->sess_ctx;
    uint8_t *rptr = (uint8_t*)buf;

    if (data_queue==NULL)
        return -1;

    do {
        ESP_LOGI(TAG,"Read via websocket, req %p queue %p size %d", req, req->sess_ctx, size);

        if (queue__receive(data_queue, &data, 10)==pdTRUE) {

            ESP_LOGI(TAG,"De-queued data size %d", data.len);

            size_t tocopy = data.len>size ? size : data.len;

            memcpy(rptr, data.data, tocopy);
            ESP_LOGI(TAG,"Copied %d (size %d)", tocopy, size);
            bytes_read += tocopy;

            /*
             Two things can happen now.
             If the stream is blocking, and we still have data remaining, we need to redo this loop;
             */
            if (block && tocopy < size) {
                size -= tocopy;
                rptr+=tocopy;
                ESP_LOGI(TAG,"Short read - waiting");
                free(data.data);
                continue;
            }

            /* if we have data not read, then we need to push it back into the queue */
            if (size < data.len) {
                ESP_LOGI(TAG,"Pushing back %d bytes", data.len-size);

                memmove(data.data, &data.data[tocopy], data.len-size);
                data.len -= size;
                size -= tocopy;
                queue__send_to_front(data_queue, &data, portMAX_DELAY);
                // Do not free data at this point!
                break;
            }
            size-=tocopy;
            rptr+=tocopy;
        } else {
            //ESP_LOGE(TAG,"Cannot read from queue!");
            //return -1;
        }
    } while (size>0);

    ESP_LOGI(TAG,"Completed read size %d", bytes_read);
    return bytes_read;
}


esp_err_t firmware_ws__firmware_upgrade(httpd_req_t *req)
{
    esp_err_t r = ESP_OK;
    uint8_t payload[512];
    httpd_ws_frame_t frame;

    static unsigned totalbytes = 0;

    memset(&frame,0,sizeof(frame));

    ESP_LOGI(TAG,"WS Callback");
    frame.payload = payload;

    r = httpd_ws_recv_frame(req, &frame, sizeof(payload));
    if (r!=ESP_OK) {
        ESP_LOGE(TAG,"Cannot read websocket data");
        return r;
    }
    totalbytes += frame.len;
    ESP_LOGI(TAG,"WS frame opcode %d len %d so far=%d", frame.type, frame.len, totalbytes);

    switch (frame.type) {
    case HTTPD_WS_TYPE_TEXT:
        if (frame.len==5 && memcmp(frame.payload,"START",5)==0)
            r = firmware_ws__start_process(req);
        else
            r = -1;
        break;
    case HTTPD_WS_TYPE_BINARY:

        if (req->sess_ctx) {
            struct wsdata data;
            Queue queue = (Queue)req->sess_ctx;
            data.len = frame.len;
            data.data = malloc(data.len);
            memcpy(data.data, payload, data.len);
            ESP_LOGI(TAG,"Queueing WS data");
            while (queue__send(queue, &data, 100)!=OS_TRUE) {
                ESP_LOGE(TAG,"Queue data delayed!!");
            }
            ESP_LOGI(TAG,"Queued WS data successfully");
        }

        break;
    default:
        break;
    }

    return r;
}

/* This uses "quick" JSON replies to save CPU processing speed */

static void firmware_ws__send_text_frame(httpd_req_t *req, const char *str, size_t len)
{
    httpd_ws_frame_t frame;
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.len = len;
    frame.payload = (unsigned char*)str;
    frame.final = true;

    httpd_ws_send_frame(req, &frame);
}

void firmware_ws__report_start(void*user)
{
    httpd_req_t *req = (httpd_req_t*)user;
    const char *start = "{\"action\":\"Starting firmware update\"}";

    firmware_ws__send_text_frame(req, start, __builtin_strlen(start));
}

void firmware_ws__report(void*user, percent_t percent)
{
    httpd_req_t *req = (httpd_req_t*)user;
    char text[128];

    if (percent<0)
        return;

    size_t len = sprintf(text, "{\"percent\":%d}", percent);

    firmware_ws__send_text_frame(req, text, len);
}

void firmware_ws__report_action(void*user, percent_t percent, progress_level_t level, const char *str)
{
    httpd_req_t *req = (httpd_req_t*)user;
    char text[128];
    size_t len;
    if (percent<0) {
        len = sprintf(text, "{\"level\":\"%s\", \"action\":\"%s\"}", progress__level_to_text(level), str);
    } else {
        len = sprintf(text, "{\"percent\":%d,\"level\":\"%s\", \"action\":\"%s\"}", percent, progress__level_to_text(level), str);
    }
    firmware_ws__send_text_frame(req, text, len);
}
void firmware_ws__report_phase_action(void*user, percent_t percent, const char *phase, progress_level_t level, const char *str)
{
    httpd_req_t *req = (httpd_req_t*)user;
    char text[256];
    text[0] = '{';
    char *tptr=&text[1];

    if (percent>=0) {
        tptr += sprintf(tptr, "\"percent\":%d,", percent);
    }
    tptr+=sprintf(tptr,"\"level\":\"%s\",\"phase\":\"%s\"", progress__level_to_text(level), phase);
    if (str) {
        tptr+=sprintf(tptr,",\"action\":\"%s\"", str);
    }
    *tptr++='}';

    firmware_ws__send_text_frame(req, text, tptr-text);
}

static const progress_reporter_t ws_reporter = {
    &firmware_ws__report_start,
    &firmware_ws__report,
    &firmware_ws__report_action,
    &firmware_ws__report_phase_action
};


