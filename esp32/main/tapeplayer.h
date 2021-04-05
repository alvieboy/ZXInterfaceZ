#ifndef __TAPEPLAYER_H__
#define __TAPEPLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
struct stream;

void tapeplayer__init(void);
void tapeplayer__stop(void);
void tapeplayer__play_file(const char *filename);
void tapeplayer__play_stream(struct stream*, size_t len, bool is_tzx, int initial_delay);
bool tapeplayer__isplaying(void);

#ifdef __cplusplus
}
#endif

#endif

