#include "esp_log.h"
#include <stdarg.h>

static void (*extern_logger)(const char *type, const char *tag, char *fmt, va_list ap) = 0;


void set_logger(void (*l)(const char *type, const char *tag, char *fmt, va_list ap))
{
    extern_logger = l;
}




void do_log(const char *type, const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (extern_logger) {
        extern_logger(type, tag, (char*)fmt,ap);
    } else {
        printf("%s %s ", type, tag);
        vprintf(fmt, ap);
        printf("\n");
    }
    va_end(ap);
}

#define do_log(m, tag,x...) do { \
} while (0)
