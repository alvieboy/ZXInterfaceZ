#ifndef __POKE_H__
#define __POKE_H__

#include <stdio.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef int (*poke_value_ask_fun_t)(void *user);
typedef int (*poke_mem_write_fun_t)(void *user, uint8_t bank, uint16_t address, uint8_t value);

typedef char pokeline_t[128];

typedef struct {
    FILE *f;
    poke_value_ask_fun_t ask_fun;
    void *ask_fun_user;
    poke_mem_write_fun_t write_fun;
    void *write_fun_user;
} poke_t;


void poke__init(poke_t *);

int poke__openfile(poke_t *poke, const char *);
int poke__loadentries(poke_t *, void (*handler)(void *, const char *), void *user);
int poke__setaskfunction(poke_t *, poke_value_ask_fun_t askfun, void *user);
int poke__setmemorywriter(poke_t *, poke_mem_write_fun_t askfun, void *user);
int poke__apply_trainer(poke_t *, const char *name);
void poke__close(poke_t *);

#ifdef __cplusplus
}
#endif

#endif
