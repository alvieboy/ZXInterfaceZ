#ifndef __FASTTAP_H__
#define __FASTTAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include "memlayout.h"
#include "model.h"

#define FASTTAP_ADDRESS_STATUS MEMLAYOUT_TAPE_WORKAREA
#define FASTTAP_ADDRESS_LENLSB (FASTTAP_ADDRESS_STATUS+1)
#define FASTTAP_ADDRESS_LENMSB (FASTTAP_ADDRESS_STATUS+2)
#define FASTTAP_ADDRESS_DATA   (FASTTAP_ADDRESS_STATUS+3)

struct fasttap_ops;
typedef struct fasttap {
    const struct fasttap_ops *ops;
    int fd;
    size_t size;
} fasttap_t;


struct fasttap_ops
{
    int (*next)(struct fasttap*, uint8_t type, uint16_t size);
    int (*init)(struct fasttap*);
    void (*stop)(struct fasttap*);
    void (*free)(struct fasttap*);
    bool (*finished)(struct fasttap*);
};


void fasttap__init();
int fasttap__prepare(const char *filename);
int fasttap__next(uint8_t type, uint16_t len);
void fasttap__stop(void);
bool fasttap__is_playing(void);
int fasttap__install_hooks(model_t model);
bool fasttap__is_file_eof(fasttap_t *fasttap);

#ifdef __cplusplus
}
#endif

#endif

