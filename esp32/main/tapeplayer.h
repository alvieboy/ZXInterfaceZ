#ifndef __TAPEPLAYER_H__
#define __TAPEPLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

void tapeplayer__init(void);
void tapeplayer__stop(void);
void tapeplayer__play(const char *filename);
bool tapeplayer__isplaying(void);

#ifdef __cplusplus
}
#endif

#endif

