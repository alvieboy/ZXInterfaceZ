#ifndef __FASTTAP_H__
#define __FASTTAP_H__

#ifdef __cplusplus
extern "C" {
#endif

int fasttap__prepare(const char *filename);
int fasttap__next(void);
void fasttap__stop(void);
int fasttap__is_playing(void);

#ifdef __cplusplus
}
#endif

#endif

