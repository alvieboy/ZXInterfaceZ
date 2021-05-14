#include "esp_log.h"
#include <stdarg.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/signal.h>

extern int ptyfd;

static void (*extern_logger)(int level, const char *tag, char *fmt, va_list ap) = 0;

uint32_t esp_log_timestamp(void)
{
    return 0;
}

void set_logger(void (*l)(int level, const char *tag, char *fmt, va_list ap))
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

static pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;

void esp_log_writev(esp_log_level_t level, const char *tag, const char *fmt, va_list ap)
{
    char line[512];
    char *ptr = line;
#if 0
    sigset_t xSignalToBlock, xOldSet;

    sigemptyset(&xSignalToBlock);
    sigaddset(&xSignalToBlock, SIG_SUSPEND);
    sigaddset(&xSignalToBlock, SIG_RESUME);
    sigaddset(&xSignalToBlock, SIG_TICK);

    pthread_sigmask(SIG_BLOCK, &xSignalToBlock, NULL);
#endif

    pthread_mutex_lock(&logmutex);

    if (extern_logger) {
        extern_logger((int)level, tag, (char*)fmt,ap);
    }

    pthread_mutex_unlock(&logmutex);
#if 0
    pthread_sigmask(SIG_UNBLOCK, &xSignalToBlock, NULL);
#endif
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
