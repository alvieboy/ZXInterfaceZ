#ifndef __DISASSEMBLE_H__
#define __DISASSEMBLE_H__

#include <inttypes.h>


#ifdef __cplusplus
extern "C" {
#endif


extern const uint16_t disassemble_tables[3][256];
extern const char *const disassemble_ops[];
extern const char *const disassemble_args[];

struct insndecode_context
{
    const uint8_t *disassembly_data;
    uint8_t hlixiyindex;
    uint8_t stringidx;
    uint8_t bytestringidx;
    uint16_t pc;
    char argstring[32];
    const char *op;
    char bytes[12];
};

const uint8_t *disassemble__decode(struct insndecode_context *ctx, const uint8_t *d, uint16_t pc);


#ifdef __cplusplus
}
#endif

#endif
