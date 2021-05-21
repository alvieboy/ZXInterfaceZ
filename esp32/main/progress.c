#include "progress.h"

static const char *progress_text[] = {
    "debug", "info", "warn", "error"
};

const char *progress__level_to_text(progress_level_t level)
{
    int ilevel = (int)level;
    if ( (ilevel<0) || ilevel>(int)PROGRESS_LEVEL_ERROR)
        ilevel=(int)PROGRESS_LEVEL_ERROR;
    return progress_text[ilevel];
}
