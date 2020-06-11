#ifndef __NVS_H__
#define __NVS_H__

#include "inttypes.h"

#define STORAGE_NAMESPACE "intz"

#define NVS_NO_HANDLE (0xFFFFFFFF)

void nvs__init(void);
int nvs__fetch_str(const char *key, char *value, unsigned maxlen, const char* def);

int nvs__fetch_i32(const char *key, int32_t *value, int32_t def);
int nvs__fetch_u32(const char *key, uint32_t *value, uint32_t def);
int nvs__fetch_u8(const char *key, uint8_t *value, uint8_t def);

uint32_t nvs__u32(const char *key, uint32_t def);
uint8_t nvs__u8(const char *key, uint8_t def);
int nvs__str(const char *key, char *value, unsigned maxlen, const char* def);

void nvs__close();




#endif
