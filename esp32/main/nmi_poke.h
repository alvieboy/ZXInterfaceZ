#ifndef __NMI_POKE_H__
#define __NMI_POKE_H__

#include <inttypes.h>

#define MAX_POKES_PER_TRAINER 12
#define BYTES_PER_POKE 5

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nmi_handler_poke
{
    uint8_t pokebuf[MAX_POKES_PER_TRAINER * BYTES_PER_POKE];
    uint8_t pokeidx;
} nmi_handler_poke_t;

void nmi_poke__init(nmi_handler_poke_t *npoke);
int nmi_poke__mem_write_fun(void *user, uint8_t bank, uint16_t address, uint8_t value);
int nmi_poke__finish(nmi_handler_poke_t *npoke);

#ifdef __cplusplus
}
#endif


#endif
