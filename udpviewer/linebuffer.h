#ifndef __LINEBUFFER_H__
#define __LINEBUFFER_H__

typedef struct {
    char *data;
    int max;
    int current;
} linebuffer_t;

void linebuffer__new(linebuffer_t *lb, char *buffer, int maxsize);
void linebuffer__reset(linebuffer_t *);
void linebuffer__init(linebuffer_t *, void (*cb)(const char data, int len));
void linebuffer__append(linebuffer_t *, const char *data, int size);


#endif
