#include "intel.h"

extern void flush_gdt(uint16_t cs, uint16_t ds);

static struct tss TSS = {
    .iomap_base = sizeof(TSS)
};

[[gnu::aligned(16)]]
static union gdt_entry GDT[GDT_IDX_MAX] = { 
    [GDT_IDX_NULL] = {},
    [GDT_IDX_CODE64] = {
        .segment_descriptor = {
            .a = 1,
            .rw = 1,
            .dc = 0,
            .e = 1,
            .s = 1,
            .dpl = PL_KERNEL,
            .p = 1,
            .l = 1,
            .db  = 0,
            .g = 1
        }
    },
    [GDT_IDX_DATA64] = {
        .segment_descriptor = {
            .a = 1,
            .rw = 1,
            .dc = 0,
            .e = 0,
            .s = 1,
            .dpl = PL_KERNEL,
            .p = 1,
            .l = 0,
            .db = 1,
            .g = 1
        }
    },
    [GDT_IDX_TSS] = {
        .system_descriptor_low = {
            .limit_low = (sizeof(TSS) - 1) & 0xFFFF,
            .type = DT_SYS_TYPE_TSS64_AVAIL,
            .s = 0,
            .dpl = PL_KERNEL,
            .p = 1,
            .limit_high = ((sizeof(TSS) - 1) >> 16) & 0xF,
        }
    },
    [GDT_IDX_TSS+1] = {
        .system_descriptor_high = {
        }
    }
};


void load_gdt(void) {
    GDT[GDT_IDX_TSS].system_descriptor_low.base_low         = (((uintptr_t)&TSS) >>  0) & 0xFF'FFFF;
    GDT[GDT_IDX_TSS].system_descriptor_low.base_high        = (((uintptr_t)&TSS) >> 24) & 0xFF;
    GDT[GDT_IDX_TSS+1].system_descriptor_high.base_veryhigh = (((uintptr_t)&TSS) >> 32) & 0xFFFF'FFFF;

    const struct dtr gdtr = {
        .limit = sizeof(GDT) - 1,
        .addr = (uintptr_t)&GDT,
    };
    __asm("lgdt %0" : : "m"(gdtr));
    __asm("lldt %0" : : "rm"(MAKE_DESRIPTOR(GDT_IDX_NULL, PL_KERNEL)));
    __asm("ltr  %0" : : "rm"(MAKE_DESRIPTOR(GDT_IDX_TSS, PL_KERNEL)));
    flush_gdt(MAKE_DESRIPTOR(GDT_IDX_CODE64, PL_KERNEL), MAKE_DESRIPTOR(GDT_IDX_NULL, PL_KERNEL));                        
}
