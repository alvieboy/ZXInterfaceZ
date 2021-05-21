#ifndef __PROGRESS_H__
#define __PROGRESS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int percent_t;

typedef enum {
    PROGRESS_LEVEL_NONE,
    PROGRESS_LEVEL_DEBUG,
    PROGRESS_LEVEL_INFO,
    PROGRESS_LEVEL_WARN,
    PROGRESS_LEVEL_ERROR
} progress_level_t;

typedef struct {
    void (*start)(void*user);
    void (*report)(void*user, percent_t);
    void (*report_action)(void*user, percent_t percent, progress_level_t level, const char *);
    void (*report_phase_action)(void*user, percent_t percent, const char *phase, progress_level_t level, const char *);
} progress_reporter_t;

const char *progress__level_to_text(progress_level_t level);

#ifdef __cplusplus
}
#endif

#endif
