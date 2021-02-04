#ifndef __SAVE_H__
#define __SAVE_H__

#ifdef __cplusplus
extern "C" {
#endif

void save__notify_save_to_tap(const char *path, const char *filename);
void save__notify_no_save(void);


#ifdef __cplusplus
}
#endif


#endif

