#include "esp_log.h"
#include <stdarg.h>

extern int ptyfd;

static void (*extern_logger)(const char *type, const char *tag, char *fmt, va_list ap) = 0;


void set_logger(void (*l)(const char *type, const char *tag, char *fmt, va_list ap))
{
    extern_logger = l;
}




void do_log(const char *type, const char *tag, const char *fmt, ...)
{
    char line[512];
    char *ptr = line;

    va_list ap;
    va_start(ap, fmt);
    if (extern_logger) {
        extern_logger(type, tag, (char*)fmt,ap);
    }
    va_end(ap);
    va_start(ap, fmt);
   // printf("%p ", ptr);
    ptr+=sprintf(ptr, "%s %s ", type, tag);
   // printf("%p ", ptr);
    ptr+=vsprintf(ptr, fmt, ap);
    *ptr++='\r';
    *ptr++='\n';
   // printf("%p ", ptr);
    fwrite(line, ptr-line, 1, stdout);

    va_end(ap);
#if 1
    if (ptyfd>=0) {
        //
        write(ptyfd, line, ptr-line);
    }
#endif
}

#define do_log(m, tag,x...) do { \
} while (0)
