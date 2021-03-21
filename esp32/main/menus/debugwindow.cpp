#include "debugwindow.h"
#include <cstring>
#include "fixedlayout.h"
#include "reg16.h"
#include "button.h"
#include "disassemble.h"
#include "memlayout.h"
#include "fpga.h"
#include "disasm.h"
#include "align.h"
/*
 visual

   0000000000111111111122222222
   0123456789012345678901234567
     PC:FFFC  SP:FFFF  AF:FFFF
     BC:FFFC  DE:FFFF  HL:FFFF
     IX:FFFC  IY:FFFF

    000b LD ($5C5F),HL
    pppp JP   PO,+128 ($FEFE)
 */

DebugWindow::DebugWindow(const nmi_cpu_context_extram &context): Window("Debugger", 30, 22)
{
    int ridx = 0;
    m_context = context;
    int line = 0;

    m_layout = WSYSObject::create<FixedLayout>();

    m_disasm = WSYSObject::create<Disasm>();

    m_layout->addChild(m_disasm, 0, line, 28, m_disasm->getMinimumHeight());

    line +=  m_disasm->getMinimumHeight() + 1;


    m_regs16[ridx] = WSYSObject::create<Reg16>("PC");
    m_layout->addChild(m_regs16[ridx++], 0, line, 8, 1);

    m_regs16[ridx] = WSYSObject::create<Reg16>("SP");
    m_layout->addChild(m_regs16[ridx++], 9, line, 8, 1);

    m_regs16[ridx] = WSYSObject::create<Reg16>("AF");
    m_layout->addChild(m_regs16[ridx++], 18, line, 8, 1);

    line++;

    m_regs16[ridx] = WSYSObject::create<Reg16>("BC");
    m_layout->addChild(m_regs16[ridx++], 0, line, 8, 1);

    m_regs16[ridx] = WSYSObject::create<Reg16>("DE");
    m_layout->addChild(m_regs16[ridx++], 9, line, 8, 1);

    m_regs16[ridx] = WSYSObject::create<Reg16>("HL");
    m_layout->addChild(m_regs16[ridx++], 18, line, 8, 1);

    line++;

    m_regs16[ridx] = WSYSObject::create<Reg16>("IX");
    m_layout->addChild(m_regs16[ridx++], 0, line, 8, 1);

    m_regs16[ridx] = WSYSObject::create<Reg16>("IY");
    m_layout->addChild(m_regs16[ridx++], 9, line, 8, 1);

    m_regs16[ridx] = WSYSObject::create<Reg16>("AF'");
    m_layout->addChild(m_regs16[ridx++], 18, line, 8, 1);

    line++;
#if 1
    m_regs16[ridx] = WSYSObject::create<Reg16>("BC'");
    m_layout->addChild(m_regs16[ridx++], 0, line, 8, 1);

    m_regs16[ridx] = WSYSObject::create<Reg16>("DE'");
    m_layout->addChild(m_regs16[ridx++], 9, line, 8, 1);

    m_regs16[ridx] = WSYSObject::create<Reg16>("HL'");
    m_layout->addChild(m_regs16[ridx++], 18, line, 8, 1);
#endif
    line++;

    Button *b = WSYSObject::create<Button>("Close");

    m_layout->addChild(b, 0, line, 10, 1);

    b->clicked().connect(static_cast<Window*>(this), &Window::destroy);

    updateRegs();
    setChild(m_layout);

    requestMem();

}
void DebugWindow::checkNextRequest()
{
    ESP_LOGI("DEBUG", "Requests pending: %d\n", m_num_requests);

    if (m_num_requests) {
        m_num_requests--;
        doRequest(&m_req[m_num_requests-1]);
    } else {
        readCompleted(m_memblockidx);
    }
}

void DebugWindow::doRequest(const struct memdata_request*r)
{
    uint32_t *aligned_buffer;

    switch (r->type) {
    case memdata_request::ROM:
        ESP_LOGI("DEBUG", "Request ROM len %d\n", r->len);
        wsys__requestromread(m_context.PC, r->len, &DebugWindow::memoryreadcomplete, this);
        break;
    case memdata_request::RAM:
        ESP_LOGI("DEBUG", "Request RAM len %d\n", r->len);
        wsys__requestmemread(m_context.PC, r->len, &DebugWindow::memoryreadcomplete, this);
        break;
    case memdata_request::SAVED_RAM:
        ESP_LOGI("DEBUG", "Request SAVED_RAM len %d\n", r->len);
        // Ensure we have an aligned buffer.
        aligned_buffer = (uint32_t*)malloc(ALIGN(r->len,4));

        fpga__read_extram_block(
                                MEMLAYOUT_NMI_SCREENAREA + r->address - 0x4000,
                                aligned_buffer,
                                r->len
                               );
        memcpy(&m_memblock[m_memblockidx], aligned_buffer, r->len);
        free(aligned_buffer);

        m_memblockidx += r->len;
        m_num_requests--;
        checkNextRequest();
        break;
    default:
        break;
    }
}

void DebugWindow::requestMem()
{
    m_memblockidx = 0;

    uint8_t size = memdata__analyse_request(m_context.PC,
                                            MAX_MEM_BLOCK,  // Worst case encoding, 4 bytes per instruction
                                            m_req,
                                            &m_num_requests);

    doRequest(&m_req[m_num_requests-1]);

}

void DebugWindow::updateRegs()
{
    int ridx = 0;
    m_regs16[ridx++]->setValue( m_context.PC );
    m_regs16[ridx++]->setValue( m_context.SP );
    m_regs16[ridx++]->setValue( m_context.AF );
    m_regs16[ridx++]->setValue( m_context.BC );
    m_regs16[ridx++]->setValue( m_context.DE );
    m_regs16[ridx++]->setValue( m_context.HL );
    m_regs16[ridx++]->setValue( m_context.IX );
    m_regs16[ridx++]->setValue( m_context.IY );
#if 1
    m_regs16[ridx++]->setValue( m_context.AF_alt );
    m_regs16[ridx++]->setValue( m_context.BC_alt );
    m_regs16[ridx++]->setValue( m_context.DE_alt );
    m_regs16[ridx++]->setValue( m_context.HL_alt );
#endif
}

void DebugWindow::memoryreadcomplete(void *self, uint8_t size)
{
    static_cast<DebugWindow*>(self)->chunkReadCompleted(size);
}

void DebugWindow::chunkReadCompleted(uint8_t len)
{
    m_num_requests--;
    ESP_LOGI("DEBUG", "Chunk completed");
    checkNextRequest();
}

void DebugWindow::readCompleted(uint8_t len)
{
    ESP_LOGI("DEBUG", "Loaded data %d\n", len);
    int lines = 6;
    uint8_t actual_len;
    uint16_t startpc = m_context.PC;

    struct insndecode_context ctx;

    const uint8_t *data = memdata__retrieve(&actual_len);

    m_disasm->clear();
    do {
        const uint8_t *end = disassemble__decode(&ctx, data, startpc);
        ESP_LOGI("DEBUG","Disassembled %s\n", ctx.argstring);
        m_disasm->add(&ctx);
        unsigned bytes_eaten  = end-data;
        startpc += bytes_eaten;
        data += bytes_eaten;
    } while (lines--);

}
