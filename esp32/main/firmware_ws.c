#include "firmware_ws.h"
#include "log.h"
#include "os/task.h"
#include "os/queue.h"
#include "os/semaphore.h"
#include "os/core.h"
#include "stream.h"
#include "firmware.h"
#include "interfacez_tasks.h"
#include "defs.h"
#include "webserver.h"

#define TAG "FIRMWAREWS"

#define WS_STATUS_ONLY_ON_REQUEST

struct wsdata {
    unsigned len;
    uint8_t *data;
};

struct firmware_ws_context {
    httpd_req_t *req;
    Semaphore sem;
    bool valid;
    Queue queue; // Main message queue
#ifdef WS_STATUS_ONLY_ON_REQUEST
    // We need to store status in a delta fashion.
    // For this, we rely on some default values to be present.
    char action[64];
    progress_level_t level; // Set to PROGRESS_LEVEL_NONE when there is no action update
    float percent; // Set to a negative value if it was not updated since.
    char phase[64];
#else
    httpd_req_t *req;
#endif
};

static const progress_reporter_t ws_reporter;

static void firmware_ws__send_text_frame(httpd_req_t *req, const char *str, size_t len);

static struct firmware_ws_context *firmware_ws__alloc_context(httpd_req_t*req)
{
    struct firmware_ws_context *ctx = malloc(sizeof(struct firmware_ws_context));

    if (NULL==ctx)
        return NULL;

    ctx->req = req;
    ctx->sem = semaphore__create_mutex();
    ctx->queue = queue__create(2, sizeof(struct wsdata));
    ctx->valid = true;

    return ctx;
}

static void firmware_ws__invalidate(struct firmware_ws_context *ctx)
{
    semaphore__take(ctx->sem, OS_MAX_DELAY);
    ctx->req = NULL;
    ctx->valid = false;
    semaphore__give(ctx->sem);
}


static void firmware_ws__free_ctx(void *data)
{
    struct firmware_ws_context *ctx = (struct firmware_ws_context*)data;
    firmware_ws__invalidate(ctx);
}


void firmware_ws__task(void *data)
{
    struct firmware_ws_context *ctx = (struct firmware_ws_context*)data;

    firmware_upgrade_t *f = (firmware_upgrade_t*)malloc(sizeof(firmware_upgrade_t));
    
    struct stream *stream = stream__alloc_websocket(ctx, NULL); // Fix finish callback

    // Setup free CTX, so we know if websocket was closed.

    httpd_sess_set_ctx( webserver__get_handle(),
                       httpd_req_to_sockfd(ctx->req),
                       ctx,
                       &firmware_ws__free_ctx);



    firmware__init(f, stream);

    stream__nonblock(stream, false);  // Force block

    // Initialise reporter
    firmware__set_reporter(f, &ws_reporter, ctx);

    ESP_LOGI(TAG,"Starting FW upgrade via websocket");

    int r = firmware__upgrade(f);

    // Destroy HTTP websocket
         /*
    httpd_sess_trigger_close(
                             webserver__get_handle(),
                             httpd_req_to_sockfd(ctx->req)
                            );
                            */

    // Wait for 2 seconds so main uploader can fetch the report
    task__delay_ms(2000);

    firmware_ws__invalidate(ctx);

    firmware__deinit(f);

    free(f);

    stream__destroy(stream);

    // destroy queue. TBD proper order..
    queue__delete(ctx->queue);
    ctx->queue = NULL;

    if (r==0)
        request_restart();


    task__delete(NULL);
}

int firmware_ws__start_process(httpd_req_t *req)
{
    /*if (firmwarews_data_queue!=NULL) {
        ESP_LOGE(TAG, "Re-entry of firmware is not possible!");
        return -1;
    } */

    struct firmware_ws_context *ctx = firmware_ws__alloc_context(req);

    /*req->sess_ctx = firmwarews_data_queue;
    req->free_ctx = &firmware_ws__free_ctx;
    */


    task__create(firmware_ws__task, "firmware_task", FIRMWAREWS_TASK_STACK_SIZE, ctx, FIRMWAREWS_TASK_PRIORITY, NULL);

    return 0;
}

int firmware_ws__read(struct firmware_ws_context *ctx, void *buf, size_t size, bool block)
{
    struct wsdata data;
    size_t bytes_read = 0;

    uint8_t *rptr = (uint8_t*)buf;

    if (ctx->valid == false || ctx->queue==NULL)
        return -1;

    do {
        ESP_LOGD(TAG,"Read via websocket, req %p queue %p size %d", ctx->req, ctx->queue, size);

        if (queue__receive(ctx->queue, &data, 10)==pdTRUE) {

            ESP_LOGD(TAG,"De-queued data size %d", data.len);

            size_t tocopy = data.len>size ? size : data.len;

            memcpy(rptr, data.data, tocopy);
            ESP_LOGD(TAG,"Copied %d (size %d)", tocopy, size);
            bytes_read += tocopy;

            /*
             Two things can happen now.
             If the stream is blocking, and we still have data remaining, we need to redo this loop;
             */
            if (block && tocopy < size) {
                size -= tocopy;
                rptr+=tocopy;
                ESP_LOGD(TAG,"Short read - waiting");
                free(data.data);
                continue;
            }

            /* if we have data not read, then we need to push it back into the queue */
            if (size < data.len) {
                ESP_LOGD(TAG,"Pushing back %d bytes", data.len-size);

                memmove(data.data, &data.data[tocopy], data.len-size);
                data.len -= size;
                size -= tocopy;
                queue__send_to_front(ctx->queue, &data, portMAX_DELAY);
                // Do not free data at this point!
                break;
            } else {
                free(data.data);
            }
            size-=tocopy;
            rptr+=tocopy;
        } else {
            //ESP_LOGE(TAG,"Cannot read from queue!");
            //return -1;
        }
        // Check if websocket was closed.

    } while (size>0);

    ESP_LOGD(TAG,"Completed read size %d", bytes_read);
    return bytes_read;
}

#ifdef WS_STATUS_ONLY_ON_REQUEST

static const char *emptystatus = "{\"percent\":0,\"phase\":\"Not started\"}";

static int firmware_ws__send_status(struct firmware_ws_context *ctx, httpd_req_t *req)
{
    char str[256];
    char *ptr = str;
    bool delim = false;
    int r = 0;

    // If process has not started, return a dummy status
    if (!ctx) {
        firmware_ws__send_text_frame(req, emptystatus, __builtin_strlen(emptystatus));
        return 0;
    }

    // Prepare text to be sent.
    semaphore__take(ctx->sem, OS_MAX_DELAY);

    *ptr++='{';
    if (ctx->percent>=0) {
        ptr+=sprintf(ptr,"\"percent\": %d", (int)ctx->percent);
        delim=true;
    }
    if (ctx->action[0] && ctx->level!=PROGRESS_LEVEL_NONE) {
        if (delim) *ptr++=',';
        ptr+=sprintf(ptr,"\"level\":\"%s\",\"action\":\"%s\"", progress__level_to_text(ctx->level), ctx->action);
        if (ctx->level==PROGRESS_LEVEL_ERROR)
            r = -1;
        delim=true;
    }

    if (ctx->phase[0]) {
        if (delim) *ptr++=',';
        ptr+=sprintf(ptr,"\"phase\":\"%s\"", ctx->phase);
    }

    *ptr++='}';
    *ptr++='\0';
    // Reset status
    ctx->percent = -1;
    ctx->level = PROGRESS_LEVEL_NONE;
    ctx->action[0] = '\0';
    ctx->phase[0] = '\0';

    if (!ctx->valid)
        r = -1;

    semaphore__give(ctx->sem);

    firmware_ws__send_text_frame(req, str, strlen(str));

    return r;
}

#endif

esp_err_t firmware_ws__firmware_upgrade(httpd_req_t *req)
{
    esp_err_t r = ESP_OK;
    uint8_t payload[512];
    httpd_ws_frame_t frame;

    static unsigned totalbytes = 0;
    struct firmware_ws_context *ctx = (struct firmware_ws_context*)req->sess_ctx;

    memset(&frame,0,sizeof(frame));

    ESP_LOGD(TAG,"WS Callback req=%p aux=%p", req, req->aux);
    frame.payload = payload;

    r = httpd_ws_recv_frame(req, &frame, sizeof(payload));
    if (r!=ESP_OK) {
        ESP_LOGE(TAG,"Cannot read websocket data");
        return r;
    }
    totalbytes += frame.len;
    ESP_LOGD(TAG,"WS frame opcode %d len %d so far=%d", frame.type, frame.len, totalbytes);

    switch (frame.type) {
    case HTTPD_WS_TYPE_TEXT:
        if (frame.len==5 && memcmp(frame.payload,"START",5)==0) {
            if (ctx) {
                r = -1;
            } else {
                r = firmware_ws__start_process(req);
            }
            if (r==0) {
                firmware_ws__send_text_frame(req, "OK", 2);
            } else {
                firmware_ws__send_text_frame(req, "FAIL", 4);
            }
        } else {

#ifdef WS_STATUS_ONLY_ON_REQUEST
            if (frame.len==6 && memcmp(frame.payload,"STATUS",6)==0) {
                r = firmware_ws__send_status(ctx, req);
            }
        }
#else
            r = -1;
        }
#endif
        break;
    case HTTPD_WS_TYPE_BINARY:

        if (ctx) {
            struct wsdata data;

            semaphore__take(ctx->sem, OS_MAX_DELAY);
            if (ctx->valid) {
                data.len = frame.len;
                data.data = malloc(data.len);
                memcpy(data.data, payload, data.len);
                ESP_LOGD(TAG,"Queueing WS data");
                while (queue__send(ctx->queue, &data, 100)!=OS_TRUE) {
                    //ESP_LOGE(TAG,"Queue data delayed!!");
                }
            }
            //ESP_LOGI(TAG,"Queued WS data successfully");
            semaphore__give(ctx->sem);

        }

        break;
    default:
        break;
    }

    return 0;
}

/* This uses "quick" JSON replies to save CPU processing speed */

static void firmware_ws__send_text_frame(httpd_req_t *req, const char *str, size_t len)
{
    httpd_ws_frame_t frame;
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.len = len;
    frame.payload = (unsigned char*)str;
    frame.final = true;

    ESP_LOGI(TAG, "Sending text frame req=%p aux=%p\n", req, req->aux);

    httpd_ws_send_frame(req, &frame);
}

void firmware_ws__report_start(void*user)
{
    struct firmware_ws_context *ctx = (struct firmware_ws_context*)user;

    semaphore__take(ctx->sem, OS_MAX_DELAY);
#ifdef WS_STATUS_ONLY_ON_REQUEST
    strcpy(ctx->action, "Starting firmware update");
    ctx->level = PROGRESS_LEVEL_INFO;
#else
    const char *start = "{\"action\":\"Starting firmware update\"}";

    if (ctx->req)
        firmware_ws__send_text_frame(ctx->req, start, __builtin_strlen(start));
#endif
    semaphore__give(ctx->sem);
}

void firmware_ws__report(void*user, percent_t percent)
{
    struct firmware_ws_context *ctx = (struct firmware_ws_context*)user;
    char text[128];

    if (percent<0)
        return;

#ifndef WS_STATUS_ONLY_ON_REQUEST
    size_t len = sprintf(text, "{\"percent\":%d}", percent);
#endif
    semaphore__take(ctx->sem, OS_MAX_DELAY);

#ifdef WS_STATUS_ONLY_ON_REQUEST
    ctx->percent = percent;
#else
    if (ctx->req)
        firmware_ws__send_text_frame(ctx->req, text, len);
#endif
    semaphore__give(ctx->sem);
}

void firmware_ws__report_action(void*user, percent_t percent, progress_level_t level, const char *str)
{
    struct firmware_ws_context *ctx = (struct firmware_ws_context*)user;
    char text[128];
    size_t len;

#ifndef WS_STATUS_ONLY_ON_REQUEST

    if (percent<0) {
        len = sprintf(text, "{\"level\":\"%s\", \"action\":\"%s\"}", progress__level_to_text(level), str);
    } else {
        len = sprintf(text, "{\"percent\":%d,\"level\":\"%s\", \"action\":\"%s\"}", percent, progress__level_to_text(level), str);
    }
#endif
    semaphore__take(ctx->sem, OS_MAX_DELAY);

#ifdef WS_STATUS_ONLY_ON_REQUEST
    if (percent>=0) {
        ctx->percent = percent;
    }
    strcpy(ctx->action, str);
    ctx->level = level;
#else
    if (ctx->req)
        firmware_ws__send_text_frame(ctx->req, text, len);
#endif
    semaphore__give(ctx->sem);
}

void firmware_ws__report_phase_action(void*user, percent_t percent, const char *phase, progress_level_t level, const char *str)
{
    struct firmware_ws_context *ctx = (struct firmware_ws_context*)user;

    char text[256];
    text[0] = '{';
    char *tptr=&text[1];
#ifndef WS_STATUS_ONLY_ON_REQUEST
    if (percent>=0) {
        tptr += sprintf(tptr, "\"percent\":%d,", percent);
    }
    tptr+=sprintf(tptr,"\"level\":\"%s\",\"phase\":\"%s\"", progress__level_to_text(level), phase);
    if (str) {
        tptr+=sprintf(tptr,",\"action\":\"%s\"", str);
    }
    *tptr++='}';
    *tptr++'\0';
#else
#endif

    semaphore__take(ctx->sem, OS_MAX_DELAY);
#ifdef WS_STATUS_ONLY_ON_REQUEST
    if (percent>=0) {
        ctx->percent = percent;
    }
    strcpy(ctx->action, str);
    strcpy(ctx->phase, phase);
    ctx->level = level;

#else
    if (ctx->req)
        firmware_ws__send_text_frame(ctx->req, text, tptr-text);
#endif
    semaphore__give(ctx->sem);
}

static const progress_reporter_t ws_reporter = {
    &firmware_ws__report_start,
    &firmware_ws__report,
    &firmware_ws__report_action,
    &firmware_ws__report_phase_action
};


