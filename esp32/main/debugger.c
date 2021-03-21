#include "debugger.h"
#include "fpga.h"
#include "memlayout.h"
#include "log.h"
#include "struct_assert.h"


#define TAG "DEBUGGER"

int debugger__load_context_from_extram(struct nmi_cpu_context_extram *target)
{
    int r;
    r = fpga__read_extram_block(MEMLAYOUT_NMI_BLOCK1, &target->w32_1[0], sizeof(target->block1));
    if (r<0)
        return r;
    r = fpga__read_extram_block(MEMLAYOUT_NMI_BLOCK2, &target->w32_2[0], sizeof(target->block2));

    target->SP += 4;

    return r;
}

ASSERT_STRUCT_SIZE(struct nmi_cpu_context_extram, 32);

void debugger__dump()
{
    struct nmi_cpu_context_extram data;
    if (debugger__load_context_from_extram(&data)<0) {
        ESP_LOGE(TAG,"Cannot load context");
        return;
    }
    debugger__dumpregs(&data);
}

void debugger__dumpregs(const struct nmi_cpu_context_extram *data)
{
#define DUMPREG16(x) \
    ESP_LOGI(TAG, " %s 0x%04x", #x, data->x)

    DUMPREG16(PC);
    DUMPREG16(SP);
    DUMPREG16(AF);
    DUMPREG16(BC);
    DUMPREG16(DE);
    DUMPREG16(HL);
    DUMPREG16(AF_alt);
    DUMPREG16(BC_alt);
    DUMPREG16(DE_alt);
    DUMPREG16(HL_alt);
}


