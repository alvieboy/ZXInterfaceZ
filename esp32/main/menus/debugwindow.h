#include "window.h"
#include "slider.h"
#include "debugger.h"
#include "memdata.h"

class Reg16;
class FixedLayout;
class Disasm;

#define NUM_LINES 8
#define MAX_MEM_BLOCK NUM_LINES*5

class DebugWindow: public Window
{
public:
    DebugWindow(const nmi_cpu_context_extram &context);
    static void memoryreadcomplete(void *, uint8_t size);
    void readCompleted(uint8_t);
    void chunkReadCompleted(uint8_t);
protected:
private:
    void checkNextRequest();
    void updateRegs();
    void requestMem();
    void doRequest(const memdata_request*r);

    FixedLayout *m_layout;

    Reg16 *m_regs16[12];

    Disasm *m_disasm;

    nmi_cpu_context_extram m_context;
    struct memdata_request m_req[2];
    uint8_t m_num_requests;
    uint8_t m_memblock[MAX_MEM_BLOCK];
    uint8_t m_memblockidx;

};
