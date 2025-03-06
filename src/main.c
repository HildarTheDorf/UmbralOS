#include <limine.h>

#include <limits.h>
#include <stdbool.h>

#define MAKE_DESRIPTOR(idx, pl) (uint16_t)(idx << 3 | pl)

#define GDT_IDX_NULL 0
#define GDT_IDX_CODE64 1
#define GDT_IDX_DATA64 2
#define GDT_IDX_TSS 3
#define GDT_IDX_MAX 5

#define GDT_SYS_TYPE_LDT 0x2
#define GDT_SYS_TYPE_TSS64_AVAIL 0x9
#define GDT_SYS_TYPE_TSS64_BUSY 0xB
#define GDT_SYS_TYPE_TSS64_BUSY 0xB
#define GDT_SYS_TYPE_CALL64 0xC
#define GDT_SYS_TYPE_INTR64 0xE
#define GDT_SYS_TYPE_TRAP64 0xF

#define IDT_IDX_MAX 256

#define PL_KERNEL 0
#define PL_USER 3

struct __attribute((packed)) dtr {
    uint16_t limit;
    uint64_t addr;
};

struct __attribute((packed, aligned(4))) gdt_segment_descriptor {
    uint64_t limit_low  :16;
    uint64_t base_low   :24;
    uint64_t a          : 1;
    uint64_t rw         : 1;
    uint64_t dc         : 1;
    uint64_t e          : 1;
    uint64_t s          : 1;
    uint64_t dpl        : 2;
    uint64_t p          : 1;
    uint64_t limit_high : 4;
    uint64_t avl        : 1;
    uint64_t l          : 1;
    uint64_t db         : 1;
    uint64_t g          : 1;
    uint64_t base_high  : 8;
};

struct __attribute((packed, aligned(4))) gdt_system_descriptor_low {
    uint64_t limit_low  :16;
    uint64_t base_low   :24;
    uint64_t type       : 4;
    uint64_t s          : 1;
    uint64_t dpl        : 2;
    uint64_t p          : 1;
    uint64_t limit_high : 4;
    uint64_t avl        : 1;
    uint64_t reserved   : 2;
    uint64_t g          : 1;
    uint64_t base_high  : 8;
};

struct __attribute((packed, aligned(4))) gdt_system_descriptor_high {
    uint64_t base_veryhigh : 32;
    uint64_t reserved      : 32;
};

union gdt_entry {
    struct gdt_segment_descriptor segment_descriptor;
    struct gdt_system_descriptor_low system_descriptor_low;
    struct gdt_system_descriptor_high system_descriptor_high;
};

struct __attribute((packed, aligned(4))) idt_entry {
    uint64_t offset_low       :16;
    uint64_t segment_selector :16;
    uint64_t ist              : 3;
    uint64_t reserved0        : 5;
    uint64_t type             : 4;
    uint64_t reserved1        : 1;
    uint64_t dpl              : 2;
    uint64_t p                : 1;
    uint64_t offset_high      :48;
    uint64_t reserved2        :32;
};

struct __attribute((packed, aligned(4))) tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
};

extern void flush_gdt(uint16_t cs, uint16_t ds);
extern void interrupt_handler(void);

LIMINE_REQUESTS_START_MARKER
LIMINE_BASE_REVISION(3)
LIMINE_REQUESTS_END_MARKER

static struct tss TSS = {
    .iomap_base = sizeof(TSS)
};

__attribute((aligned(16)))
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
            .type = GDT_SYS_TYPE_TSS64_AVAIL,
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

__attribute((aligned(16)))
static struct idt_entry IDT[IDT_IDX_MAX] = {

};

[[noreturn]] static void halt() {
    __asm("cli");
    while (true) {
        __asm("hlt");
    }
}

static void load_gdt(void) {
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
    flush_gdt(MAKE_DESRIPTOR(GDT_IDX_CODE64, PL_KERNEL), MAKE_DESRIPTOR(GDT_IDX_DATA64, PL_KERNEL));                        
}

static void load_idt(void) {
    for (int i = 0; i < IDT_IDX_MAX; ++i) {
        IDT[i].offset_low = (((uintptr_t)&interrupt_handler) >> 0) & 0xFFFF;
        IDT[i].segment_selector = MAKE_DESRIPTOR(GDT_IDX_CODE64, PL_KERNEL);
        IDT[i].ist = 0;
        IDT[i].type = GDT_SYS_TYPE_INTR64;
        IDT[i].p = 1;
        IDT[i].offset_high = (((uintptr_t)&interrupt_handler) >> 16) & 0xFFFF'FFFF'FFFF;
    }

    const struct dtr idtr = {
        .limit = sizeof(IDT) - 1,
        .addr = (uintptr_t)&IDT
    };
    __asm("lidt %0" : : "m"(idtr));
}

[[noreturn]] void _start() {
    load_gdt();         
    load_idt();

    __asm("int $3");

    halt();
}
