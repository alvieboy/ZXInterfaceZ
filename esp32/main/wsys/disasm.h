#pragma once

#include "widget.h"
#include <string>
#include <vector>

struct insndecode_context;

#define DISASM_ENTRIES 8

class Disasm: public Widget
{
public:
    Disasm(Widget *parent=NULL);
    virtual void drawImpl() override;
    virtual uint8_t getMinimumHeight() const override;
    virtual uint8_t getMinimumWidth() const override;
    void clear();
    void add(const struct insndecode_context *);
private:

    struct disasm_entry {
        uint16_t pc;
        const char *op;
        char argstr[24];
        char bytesstr[16];
    };

    std::vector<disasm_entry> m_entries;
};
