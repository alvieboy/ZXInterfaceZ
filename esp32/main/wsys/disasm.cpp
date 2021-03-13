#include "disasm.h"
#include "disassemble.h"
#include <cstring>

Disasm::Disasm(Widget *parent): Widget(parent)
{
    redraw();
}



uint8_t Disasm::getMinimumHeight() const {
    return DISASM_ENTRIES;
}

uint8_t Disasm::getMinimumWidth() const
{
    return 28;
}

void Disasm::drawImpl()
{
    char tmp[5];
    int oplen;
    parentDrawImpl();

    screenptr_t ptr = m_screenptr;
    for (auto i: m_entries) {
        screenptr_t lineptr = ptr;

        sprintf(tmp, "%04X", i.pc);
        lineptr = lineptr.drawstring(tmp);
        lineptr++;
        oplen = strlen(i.op);
        lineptr = lineptr.drawstring(i.op);
        lineptr+= (5-oplen);

        lineptr.drawstring(i.argstr);

        ptr.nextcharline();
    }

}


void Disasm::clear()
{
    m_entries.clear();
    redraw();
}

void Disasm::add(const struct insndecode_context *ctx)
{
    struct disasm_entry e;
    e.pc = ctx->pc;
    e.op = ctx->op;
    strcpy( e.argstr, ctx->argstring);
    strcpy( e.bytesstr, ctx->bytes);
    m_entries.push_back(e);
}


