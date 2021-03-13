#include "disassemble.h"
#include <stdarg.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

static const char *hlixiy[3] = {
    "HL",
    "IX",
    "IY"
};

static const char *hlixiy_high[3] = {
    "H",
    "IXH",
    "IYH"
};

static const char *hlixiy_low[3] = {
    "L",
    "IXL",
    "IYL"
};

static uint8_t read_byte(struct insndecode_context *ctx);
static uint16_t read_word(struct insndecode_context *ctx);


static void context_sprintf(struct insndecode_context *ctx, const char*fmt,...)
{
    va_list ap;
    va_start(ap, fmt);
    ctx->stringidx += vsprintf(&ctx->argstring[ctx->stringidx], fmt, ap);
    va_end(ap);
}

static void writearg(struct insndecode_context *ctx, const char *argstr)
{
    char c;
    uint16_t w16;
    uint8_t w8;
    int8_t displ;
    uint16_t newpc;

    while ((c=*argstr++)) {
        if (islower(c)) {
            switch(c) {
            case 'b':
                w8 =  read_byte(ctx);
                context_sprintf(ctx,"$%02X", w8);
                break;
            case 'd':
                displ = (int8_t)read_byte(ctx);
                newpc = ctx->pc + 2 + (int16_t)displ;
                context_sprintf(ctx, "%+d ($%04x)", displ, newpc);
                break;
            case 'w':
                w16 = read_word(ctx);
                context_sprintf(ctx,"$%04X", w16);
                break;
            case 'z':
                context_sprintf(ctx,"%s", hlixiy[ctx->hlixiyindex]);
                break;
            case 'h':
                context_sprintf(ctx,"%s", hlixiy_high[ctx->hlixiyindex]);
                break;
            case 'l':
                context_sprintf(ctx,"%s", hlixiy_low[ctx->hlixiyindex]);
                break;
            case 'x':
                if (ctx->hlixiyindex) {
                    displ = (int8_t)read_byte(ctx);
                    context_sprintf(ctx,"%s%+d", hlixiy[ctx->hlixiyindex],displ);
                } else {
                    context_sprintf(ctx,"%s", hlixiy[ctx->hlixiyindex]);
                }
                break;
            default:
                return;
            }
        } else {
            ctx->argstring[ctx->stringidx++] = c;
        }
    }
}

static void printinsn(struct insndecode_context *ctx, uint16_t val)
{
    uint8_t op = val>>8;
    uint8_t arg  = val &0xff;
    const char *argstr = disassemble_args[arg];
    ctx->op = disassemble_ops[op];
    writearg(ctx, argstr);
}

#include <stdbool.h>


static void init_context(struct insndecode_context*ctx,
                        const uint8_t *data, uint16_t pc)
{
    ctx->disassembly_data = data;
    ctx->hlixiyindex = 0;
    ctx->stringidx = 0;
    ctx->bytestringidx = 0;
    ctx->pc = pc;
}

static uint8_t read_byte(struct insndecode_context *ctx)
{
    uint8_t val = *ctx->disassembly_data;
    ctx->disassembly_data++;
    if (ctx->bytestringidx>0) {
        ctx->bytes[ctx->bytestringidx] = ' ';
        ctx->bytestringidx++;
    }
    ctx->bytestringidx += sprintf(&ctx->bytes[ctx->bytestringidx],"%02x", val);
    return val;
}

static uint16_t read_word(struct insndecode_context *ctx)
{
    uint16_t v = read_byte(ctx);
    v += ((uint16_t)read_byte(ctx))<<8;
    return v;
}

const uint8_t *disassemble__decode(struct insndecode_context *ctx, const uint8_t *d, uint16_t pc)
{
    init_context(ctx, d, pc);

    int table = 0;
    uint16_t val;
    bool more;

    do {
        more = false;
        uint8_t i = read_byte(ctx);
        val = disassemble_tables[table][i];

     //   printf("VAL %04x %02x\n", val, i);
        uint8_t op = val>>8;
        uint8_t arg  = val &0xf;

        if (table==0) {
            switch (op) {
            case 0xff: // New table
                table = arg;
                more = true;
                break;
            case 0xfe: //
                ctx->hlixiyindex = arg;
                more = true;
                break;
            default:
                break;
            }

        }
    } while (more);
    printinsn(ctx, val);

    ctx->argstring[ctx->stringidx] = '\0';
    ctx->bytes[ctx->bytestringidx] = '\0';

    //printf("%04x %s [%s]\n", pc, ctx.string, ctx.bytes);

    return ctx->disassembly_data;
}

