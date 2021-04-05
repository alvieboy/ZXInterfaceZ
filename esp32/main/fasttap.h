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

typedef enum {
    TAP_TYPE_UNKNOWN,
    TAP_TYPE_TAP,
    TAP_TYPE_TZX,
    TAP_TYPE_SCR
} tap_type_t;

struct fasttap_ops;

typedef struct fasttap {
    const struct fasttap_ops *ops;
    struct stream *stream;//int fd;
    size_t size;
    size_t read;
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
int fasttap__prepare_from_file(const char *filename);
int fasttap__prepare_from_stream(struct stream*, size_t size, tap_type_t type);
int fasttap__next(uint8_t type, uint16_t len);
void fasttap__stop(void);
bool fasttap__is_playing(void);
int fasttap__install_hooks(model_t model);
bool fasttap__is_file_eof(fasttap_t *fasttap);
tap_type_t fasttap__type_from_ext(const char *ext);

/* To be used by the TAP implementation only */
int fasttap__read(fasttap_t *tap, void *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif

