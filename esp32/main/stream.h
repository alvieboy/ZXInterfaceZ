#ifndef __STREAM_H__
#define __STREAM_H__
#include "esp_http_server.h"

struct stream;

struct stream *stream__alloc_system(int fd);
struct stream *stream__alloc_httpd(httpd_req_t*, void(*notify_finish)(httpd_req_t*) );
struct stream *stream__destroy(struct stream *);

int stream__read(struct stream *, void *buf, size_t size);
int stream__read_blocking(struct stream *, void *buf, size_t size);
bool stream__seekable(struct stream *);
off_t stream__seek(struct stream *, off_t off, int whence);
int stream__nonblock(struct stream *, bool enable);

#endif
