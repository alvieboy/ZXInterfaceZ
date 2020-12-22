#include "esp_log.h"
#include <stdarg.h>
#include <stdio.h>

extern int ptyfd;

static void (*extern_logger)(const char *type, const char *tag, char *fmt, va_list ap) = 0;

uint32_t esp_log_timestamp(void)
{
    return 0;
}


/*
void esp_log_write(esp_log_level_t level, const char* tag, const char* format, ...)
{

} */

void set_logger(void (*l)(const char *type, const char *tag, char *fmt, va_list ap))
{
    extern_logger = l;
}




void do_log(const char *type, const char *tag, const char *fmt, ...)
{
}

void esp_log_write(esp_log_level_t level, const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    esp_log_writev(level, tag, fmt, ap);
    va_end(ap);

}

void esp_log_writev(esp_log_level_t level, const char *tag, const char *fmt, va_list ap)
{
    char line[512];
    char *ptr = line;

    //va_start(ap, fmt);
    if (extern_logger) {
        extern_logger(tag, "", (char*)fmt,ap);
    }
    //va_end(ap);
    //va_start(ap, fmt);
    //vfprintf(stdout, fmt, ap);
    //va_end(ap);
#if 0
    if (ptyfd>=0) {
        //
        write(ptyfd, line, ptr-line);
    }
#endif
}

#define do_log(m, tag,x...) do { \
} while (0)
