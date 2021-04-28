#include "stream.h"
#include <fcntl.h>
#include <unistd.h>
#include "log.h"
#include "fileaccess.h"

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
    bool nonblock;
};


static int system_read(struct stream *, void *buf, size_t size);
static int system_close(struct stream *);
static int system_seek(struct stream *, off_t off, int whence);
static int system_nonblock(struct stream *, bool);

static int httpd_read(struct stream *, void *buf, size_t size);
static int httpd_close(struct stream *);
static int httpd_nonblock(struct stream *, bool);


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

static int httpd_close(struct stream *s)
{
    ESP_LOGI("STREAM", "HTTP read finished");
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


struct stream *stream__alloc_system(int fd)
{
    struct stream *s = malloc(sizeof(struct stream));
    if (!s)
        return s;
    s->ops = &system_stream_ops;
    s->fd = fd;
    return s;
}

struct stream *stream__alloc_httpd(httpd_req_t*req, void (*notify_finished)(httpd_req_t*))
{
    struct stream *s = malloc(sizeof(struct stream));
    if (!s)
        return s;
    s->ops = &httpd_stream_ops;
    s->http_req = req;
    s->notify_finished = notify_finished;
    s->nonblock = true;
    return s;
}

struct stream *stream__destroy(struct stream *s)
{
    if (s) {
        s->ops->close(s);
        free(s);
    }
    return NULL;
}

int stream__read(struct stream *s, void *buf, size_t size)
{
    return s->ops->read(s, buf, size);
}

int stream__read_blocking(struct stream *s, void *buf, size_t size)
{
    size_t remain = size;
    uint8_t *target = (uint8_t*)buf;
    do {
        int r = s->ops->read(s, target, remain);

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
