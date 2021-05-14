#include "stream.h"
#include <fcntl.h>
#include <unistd.h>
#include "log.h"
#include "fileaccess.h"
#include "firmware_ws.h"

#define STREAM_BUFFER_SIZE 512

struct streamops {
    int (*read)(struct stream *, void *buf, size_t size);
    int (*close)(struct stream *);
    int (*seek)(struct stream *, off_t off, int whence);
    int (*nonblock)(struct stream *, bool);
};

struct stream {
    const struct streamops *ops;
    union {
        int fd;
        httpd_req_t *http_req;
    };
    void (*notify_finished)(httpd_req_t*);
    uint8_t *buffer;
    unsigned buffer_low, buffer_high;
    bool nonblock;
    bool buffered;
};


static int system_read(struct stream *, void *buf, size_t size);
static int system_close(struct stream *);
static int system_seek(struct stream *, off_t off, int whence);
static int system_nonblock(struct stream *, bool);

static int httpd_read(struct stream *, void *buf, size_t size);
static int httpd_close(struct stream *);
static int httpd_nonblock(struct stream *, bool);

static int httpd_ws_read(struct stream *, void *buf, size_t size);
static int httpd_ws_close(struct stream *);
static int httpd_ws_nonblock(struct stream *, bool);


static const struct streamops system_stream_ops = {
    .read = &system_read,
    .close = &system_close,
    .seek  = &system_seek,
    .nonblock = &system_nonblock
};

static const struct streamops httpd_stream_ops = {
    .read = &httpd_read,
    .close = &httpd_close,
    .seek = NULL,
    .nonblock = &httpd_nonblock
};

static const struct streamops httpd_ws_stream_ops = {
    .read = &httpd_ws_read,
    .close = &httpd_ws_close,
    .seek = NULL,
    .nonblock = &httpd_ws_nonblock
};

static struct stream* stream__alloc()
{
    struct stream *s = calloc(1, sizeof(struct stream));
    return s;
}


static int system_read(struct stream *s, void *buf, size_t size)
{
    return __read(s->fd, buf, size);
}

static int system_close(struct stream *s)
{
    return close(s->fd);
}

static int system_seek(struct stream *s, off_t off, int whence)
{
    return lseek(s->fd, off, whence);
}

static int httpd_read_block(struct stream *s, void *buf, size_t size)
{
    int remain = size;
    int read = 0;
    char *rbuf = (char*)buf;
    do {
        int r = httpd_req_recv(s->http_req, rbuf, remain);
        if (r<0)
            return -1;
        if (r==0) {
            return read;
        }
        read += r;
        rbuf += r;
        remain -= r;
    } while (remain>0);
    return read;
}

static int httpd_read(struct stream *s, void *buf, size_t size)
{
    if (!s->nonblock)
        return httpd_read_block(s, buf, size);

    int received = httpd_req_recv(s->http_req, buf, size);
    //ESP_LOGI("STREAM", "Reading from HTTPD: %d",received);
    if (received<=0) {
        if (s->notify_finished) {
            s->notify_finished(s->http_req);
            s->notify_finished = NULL;
        }
    }
    return received;
}

static int httpd_ws_read(struct stream *s, void *buf, size_t size)
{
    ESP_LOGI("WSREAD", "read %p", s->http_req);

    return firmware_ws__read(s->http_req, buf, size, !s->nonblock);
    //return -1;
}

static int httpd_close(struct stream *s)
{
    ESP_LOGI("STREAM", "HTTP read finished");
    if (s->notify_finished) {
        s->notify_finished(s->http_req);
    }
    return 0;
}

static int httpd_ws_close(struct stream *s)
{
    ESP_LOGI("STREAM", "HTTP WS read finished");
    if (s->notify_finished) {
        s->notify_finished(s->http_req);
    }
    return 0;
}

int system_nonblock(struct stream *s, bool nonblock)
{
    int flags = fcntl(s->fd, F_GETFL);

    if (flags<0)
        return flags;

    if (nonblock)
        flags|=O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    return fcntl(s->fd, F_SETFL, flags);
}

int httpd_nonblock(struct stream *s, bool nonblock)
{
    s->nonblock = nonblock;
    return 0;
}

int httpd_ws_nonblock(struct stream *s, bool nonblock)
{
    s->nonblock = nonblock;
    return 0;
}


struct stream *stream__alloc_system(int fd)
{
    struct stream *s = stream__alloc();
    if (!s)
        return s;
    s->ops = &system_stream_ops;
    s->fd = fd;
    return s;
}

struct stream *stream__alloc_httpd(httpd_req_t*req, void (*notify_finished)(httpd_req_t*))
{
    struct stream *s = stream__alloc();
    if (!s)
        return s;
    s->ops = &httpd_stream_ops;
    s->http_req = req;
    s->notify_finished = notify_finished;
    s->nonblock = true;
    return s;
}

struct stream *stream__alloc_websocket(httpd_req_t*req, void (*notify_finished)(httpd_req_t*))
{
    struct stream *s = stream__alloc();
    if (!s)
        return s;
    s->ops = &httpd_ws_stream_ops;
    s->http_req = req;

    ESP_LOGI("STREAM","req %p", req);

    s->notify_finished = notify_finished;
    s->nonblock = true;
    return s;
}

struct stream *stream__destroy(struct stream *s)
{
    if (s) {
        s->ops->close(s);

        if (s->buffered) {
            free(s->buffer);
        }
        free(s);
    }
    return NULL;
}

int stream__read_buffered(struct stream *s, void *buf, size_t size)
{
    int r = 0;
    uint8_t *target = (uint8_t *)buf;
    // First, copy data from internal buffer if we have data available.
    unsigned avail = s->buffer_high - s->buffer_low;
    ESP_LOGI("STREAM","Stream buffered read %d buffered %d", size, avail);
    if (avail) {
        unsigned tocopy = avail > size ? size : avail;
        memcpy(target, &s->buffer[s->buffer_low], tocopy);
        size -= tocopy;
        target += tocopy;
        s->buffer_low += tocopy;
        r = tocopy;
    }
#ifdef STREAM_NO_BLOCK_READ
    if (size>0) {
        // No data in available in buffer.
        // Read out size_bytes directly.
        int lr = s->ops->read(s, target, size);
        if (lr<0) {
            if (r>0)
                return r;
            return -1; // no buffered bytes and error read.
        }
        r+=lr; // Update bytes read.

        if (r>0) {
            // Read out excess data into the local buffer
            s->buffer_low = s->buffer_high = 0;
            lr = s->ops->read(s, s->buffer, STREAM_BUFFER_SIZE);
            if (lr>0) {
                s->buffer_high = lr;
            }
        }

    }
#else
    while (size>0) {
        // No data in available in buffer.
        if (size >= STREAM_BUFFER_SIZE) {
            int lr = s->ops->read(s, target, size);
            if (lr<0) {
                if (r>0)
                    return r;
                return -1; // no buffered bytes and error read.
            }
            r+=lr;
            size-=lr;
            target+=lr;
            s->buffer_high = 0;
            s->buffer_low = 0;
            ESP_LOGI("STREAM", "Full block read");
        } else {
            ESP_LOGI("STREAM", "Short block read");
            // Less than block size read.
            // We still may or may not be able to read the whole
            // size.

            int lr = s->ops->read(s, s->buffer, STREAM_BUFFER_SIZE);

            if (lr<0) {
                return -1; // no buffered bytes and error read.
            }

            unsigned fulfilled = lr>size ? size: lr;

            s->buffer_high = lr;
            s->buffer_low = fulfilled;
            memcpy(target, s->buffer, fulfilled);

            r+=fulfilled; // Update bytes read.
            size-=fulfilled;
            target+=fulfilled;
        }
    }
#endif
    ESP_LOGI("STREAM", "Ret %d",r);
    return r;
}

int stream__read(struct stream *s, void *buf, size_t size)
{
    if (!s->buffered)
        return s->ops->read(s, buf, size);

    // Buffered read.
    return stream__read_buffered(s, buf, size);
}

int stream__set_buffering(struct stream *s, bool enabled)
{
    if (enabled && !s->buffered) {
        s->buffer = malloc(STREAM_BUFFER_SIZE);
        s->buffer_low = 0;
        s->buffer_high = 0;
    }
    if (!enabled && s->buffered) {
        free(s->buffer);
    }

    s->buffered = enabled;

    return 0;
}

int stream__read_blocking(struct stream *s, void *buf, size_t size)
{
    size_t remain = size;
    uint8_t *target = (uint8_t*)buf;
    do {
        int r = stream__read(s, target, remain);

        if (r<0)
            return -1;

        if (r==0)
            return size - remain;

        target += r;
        remain -= r;
    } while (remain>0);

    return size - remain;
}

bool stream__seekable(struct stream *s)
{
    return s->ops->seek != NULL;
}

off_t stream__seek(struct stream *s, off_t off, int whence)
{
    if (s->ops->seek) {
        return s->ops->seek(s,off, whence);
    }
    return -1;
}

int stream__nonblock(struct stream *s, bool enable)
{
    if (s->ops->nonblock) {
        return s->ops->nonblock(s, enable);
    }
    return -1;

}
