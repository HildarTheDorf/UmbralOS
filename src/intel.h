#pragma once

#include <stdint.h>

#define MAKE_DESRIPTOR(idx, pl) (uint16_t)(idx << 3 | pl)

#define DT_SYS_TYPE_LDT 0x2
#define DT_SYS_TYPE_TSS64_AVAIL 0x9
#define DT_SYS_TYPE_TSS64_BUSY 0xB
#define DT_SYS_TYPE_TSS64_BUSY 0xB
#define DT_SYS_TYPE_CALL64 0xC
#define DT_SYS_TYPE_INTR64 0xE
#define DT_SYS_TYPE_TRAP64 0xF

#define GDT_IDX_NULL 0
#define GDT_IDX_CODE64 1
#define GDT_IDX_DATA64 2
#define GDT_IDX_TSS 3
#define GDT_IDX_MAX 5

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

struct __attribute((packed, aligned(8))) page_directory_entry_subdirectory {
    uint64_t p        : 1;
    uint64_t rw       : 1;
    uint64_t us       : 1;
    uint64_t pwt      : 1;
    uint64_t pcd      : 1;
    uint64_t a        : 1;
    uint64_t avl0     : 1;
    uint64_t ps       : 1;
    uint64_t avl1     : 4;
    uint64_t addr     :40;
    uint64_t avl2     :11;
    uint64_t xd       : 1;
};

struct __attribute((packed, aligned(8))) page_directory_entry_largepage {
    uint64_t p        : 1;
    uint64_t rw       : 1;
    uint64_t us       : 1;
    uint64_t pwt      : 1;
    uint64_t pcd      : 1;
    uint64_t a        : 1;
    uint64_t d        : 1;
    uint64_t ps       : 1;
    uint64_t g        : 1;
    uint64_t avl1     : 3;
    uint64_t pat      : 1;
    uint64_t addr     : 39;
    uint64_t avl2     : 7;
    uint64_t pk       : 4;
    uint64_t xd       : 1;
};

struct __attribute((packed, aligned(8))) page_table_entry {
    uint64_t p        : 1;
    uint64_t rw       : 1;
    uint64_t us       : 1;
    uint64_t pwt      : 1;
    uint64_t pcd      : 1;
    uint64_t a        : 1;
    uint64_t d        : 1;
    uint64_t pat      : 1;
    uint64_t g        : 1;
    uint64_t avl1     : 3;
    uint64_t addr     : 40;
    uint64_t avl2     : 7;
    uint64_t pk       : 4;
    uint64_t xd       : 1;
};

