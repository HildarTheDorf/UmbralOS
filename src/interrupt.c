#include "acpi.h"
#include "intel.h"
#include "common.h"
#include "mm.h"
#include "security.h"

#include <stdint.h>

#define PIC1 0x20
#define PIC1_COMMAND (PIC1 + 0)
#define PIC1_DATA (PIC1 + 1)

#define PIC2 0xA0
#define PIC2_COMMAND (PIC2 + 0)
#define PIC2_DATA (PIC2 + 1)

#define PIC_ICW1_ICW4 0x01
#define PIC_ICW1_INIT 0x10
#define PIC_ICW4_8086 0x01

#define PIC_READ_ISR 0x0B
#define PIC_EOI 0x20

#define LAPIC_REGISTER_SIZE 0x10
#define LAPIC_REGISTER_MAX 0x40

#define LAPIC_REGISTER_IDX_EOI 0x0B
#define LAPIC_REGISTER_IDX_SPIV 0x0F
#define LAPIC_REGISTER_IDX_LVT_CMCI 0x2F
#define LAPIC_REGISTER_IDX_LVT_TIMER 0x32
#define LAPIC_REGISTER_IDX_LVT_THERM 0x33
#define LAPIC_REGISTER_IDX_LVT_PERFC 0x34
#define LAPIC_REGISTER_IDX_LVT_LINT0 0x35
#define LAPIC_REGISTER_IDX_LVT_LINT1 0x36
#define LAPIC_REGISTER_IDX_LVT_ERROR 0x37

#define LAPIC_LVT_DELIVERY_FIXED (0b000 << 8)
#define LAPIC_LVT_DELIVERY_SMI (0b010 << 8)
#define LAPIC_LVT_DELIVERY_NMI (0b100 << 8)
#define LAPIC_LVT_DELIVERY_EXTINT (0b111 << 8)
#define LAPIC_LVT_DELIVERY_INIT (0b101 << 8)

#define LAPIC_LVT_MASK (1 << 16)

#define LAPIC_SPIV_ENABLE 0x100

#define IOAPICVER 0x01
#define IOAPICREDTBL(n) (0x10 + 2 * n)

#define IOAPICVER_MAXENTRY_SHIFT 16

#define IOAPICREDTBL_POLARITY (1 << 13)
#define IOAPICREDTBL_TRIGGER (1 << 15)
#define IOAPICREDTBL_MASK (1 << 16)

#define IOAPICREDTBL_DELIVERY_FIXED(vector) (vector)
#define IOAPICREDTBL_DELIVERY_NMI (0b100 << 8)

#define FOR_EACH_INTERRUPT \
    I(0) \
    I(1) \
    I(2) \
    I(3) \
    I(4) \
    I(5) \
    I(6) \
    I(7) \
    I(8) \
    I(9) \
    I(10) \
    I(11) \
    I(12) \
    I(13) \
    I(14) \
    I(15) \
    I(16) \
    I(17) \
    I(18) \
    I(19) \
    I(20) \
    I(21) \
    I(22) \
    I(23) \
    I(24) \
    I(25) \
    I(26) \
    I(27) \
    I(28) \
    I(29) \
    I(30) \
    I(31) \
    I(32) \
    I(33) \
    I(34) \
    I(35) \
    I(36) \
    I(37) \
    I(38) \
    I(39) \
    I(40) \
    I(41) \
    I(42) \
    I(43) \
    I(44) \
    I(45) \
    I(46) \
    I(47) \
    I(48) \
    I(49) \
    I(50) \
    I(51) \
    I(52) \
    I(53) \
    I(54) \
    I(55) \
    I(56) \
    I(57) \
    I(58) \
    I(59) \
    I(60) \
    I(61) \
    I(62) \
    I(63) \
    I(64) \
    I(65) \
    I(66) \
    I(67) \
    I(68) \
    I(69) \
    I(70) \
    I(71) \
    I(72) \
    I(73) \
    I(74) \
    I(75) \
    I(76) \
    I(77) \
    I(78) \
    I(79) \
    I(80) \
    I(81) \
    I(82) \
    I(83) \
    I(84) \
    I(85) \
    I(86) \
    I(87) \
    I(88) \
    I(89) \
    I(90) \
    I(91) \
    I(92) \
    I(93) \
    I(94) \
    I(95) \
    I(96) \
    I(97) \
    I(98) \
    I(99) \
    I(100) \
    I(101) \
    I(102) \
    I(103) \
    I(104) \
    I(105) \
    I(106) \
    I(107) \
    I(108) \
    I(109) \
    I(110) \
    I(111) \
    I(112) \
    I(113) \
    I(114) \
    I(115) \
    I(116) \
    I(117) \
    I(118) \
    I(119) \
    I(120) \
    I(121) \
    I(122) \
    I(123) \
    I(124) \
    I(125) \
    I(126) \
    I(127) \
    I(128) \
    I(129) \
    I(130) \
    I(131) \
    I(132) \
    I(133) \
    I(134) \
    I(135) \
    I(136) \
    I(137) \
    I(138) \
    I(139) \
    I(140) \
    I(141) \
    I(142) \
    I(143) \
    I(144) \
    I(145) \
    I(146) \
    I(147) \
    I(148) \
    I(149) \
    I(150) \
    I(151) \
    I(152) \
    I(153) \
    I(154) \
    I(155) \
    I(156) \
    I(157) \
    I(158) \
    I(159) \
    I(160) \
    I(161) \
    I(162) \
    I(163) \
    I(164) \
    I(165) \
    I(166) \
    I(167) \
    I(168) \
    I(169) \
    I(170) \
    I(171) \
    I(172) \
    I(173) \
    I(174) \
    I(175) \
    I(176) \
    I(177) \
    I(178) \
    I(179) \
    I(180) \
    I(181) \
    I(182) \
    I(183) \
    I(184) \
    I(185) \
    I(186) \
    I(187) \
    I(188) \
    I(189) \
    I(190) \
    I(191) \
    I(192) \
    I(193) \
    I(194) \
    I(195) \
    I(196) \
    I(197) \
    I(198) \
    I(199) \
    I(200) \
    I(201) \
    I(202) \
    I(203) \
    I(204) \
    I(205) \
    I(206) \
    I(207) \
    I(208) \
    I(209) \
    I(210) \
    I(211) \
    I(212) \
    I(213) \
    I(214) \
    I(215) \
    I(216) \
    I(217) \
    I(218) \
    I(219) \
    I(220) \
    I(221) \
    I(222) \
    I(223) \
    I(224) \
    I(225) \
    I(226) \
    I(227) \
    I(228) \
    I(229) \
    I(230) \
    I(231) \
    I(232) \
    I(233) \
    I(234) \
    I(235) \
    I(236) \
    I(237) \
    I(238) \
    I(239) \
    I(240) \
    I(241) \
    I(242) \
    I(243) \
    I(244) \
    I(245) \
    I(246) \
    I(247) \
    I(248) \
    I(249) \
    I(250) \
    I(251) \
    I(252) \
    I(253) \
    I(254) \
    I(255)

struct redirection_entry {
    uint8_t destination;
    bool is_level_triggered;
    bool is_active_low;    
};

struct stack_frame {
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t rdi;
    uint64_t reserved;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

static bool HAS_X2APIC;

static void *LAPIC_BASE;
static void *IOAPIC_BASE;

static struct redirection_entry ISA_REDIRECTION_ENTRY[16] = {
    [0] = {.destination = 0},
    [1] = {.destination = 1},
    [2] = {.destination = 2},
    [3] = {.destination = 3},
    [4] = {.destination = 4},
    [5] = {.destination = 5},
    [6] = {.destination = 6},
    [7] = {.destination = 7},
    [8] = {.destination = 8},
    [9] = {.destination = 9},
    [10] = {.destination = 10},
    [11] = {.destination = 11},
    [12] = {.destination = 12},
    [13] = {.destination = 13},
    [14] = {.destination = 14},
    [15] = {.destination = 15},
};

static struct {
    uint8_t lint;
    bool is_level_triggered;
    bool is_active_low;
} LAPIC_NMI_REDIRECTION;

static struct {
    uint16_t destination;
    bool is_level_triggered;
    bool is_active_low;
    bool is_valid;
} IOAPIC_NMI_REDIRECTION;

#define I(X) extern void interrupt_stub_##X(void);
FOR_EACH_INTERRUPT
#undef I

#define I(X) [X] = { \
    .segment_selector = MAKE_DESRIPTOR(GDT_IDX_CODE64, PL_KERNEL), \
    .type = DT_SYS_TYPE_INTR64, \
    .p = 1 \
},

[[gnu::aligned(16)]]
static struct idt_entry IDT[IDT_IDX_MAX + 1] = {
    FOR_EACH_INTERRUPT
};

#undef I

static const char *vector_to_menemonic(uint8_t vector) {
    switch (vector) {
    case IDT_IDX_EXCEPTION_DE:
        return "#DE";
    case IDT_IDX_EXCEPTION_DB:
        return "#DB";
    case IDT_IDX_EXCEPTION_NMI:
        return "#NMI";
    case IDT_IDX_EXCEPTION_BP:
        return "#BP";
    case IDT_IDX_EXCEPTION_OF:
        return "#OF";
    case IDT_IDX_EXCEPTION_BR:
        return "#BR";
    case IDT_IDX_EXCEPTION_UD:
        return "#UD";
    case IDT_IDX_EXCEPTION_NM:
        return "#NM";
    case IDT_IDX_EXCEPTION_DF:
        return "#DF";
    case IDT_IDX_EXCEPTION_CSO:
        return "#CSO";
    case IDT_IDX_EXCEPTION_TS:
        return "#TS";
    case IDT_IDX_EXCEPTION_NP:
        return "#NP";
    case IDT_IDX_EXCEPTION_SS:
        return "#SS";
    case IDT_IDX_EXCEPTION_GP:
        return "#GP";
    case IDT_IDX_EXCEPTION_PF:
        return "#PF";
    case IDT_IDX_EXCEPTION_MF:
        return "#MF";
    case IDT_IDX_EXCEPTION_AC:
        return "#AC";
    case IDT_IDX_EXCEPTION_MC:
        return "#MC";
    case IDT_IDX_EXCEPTION_XM:
        return "#XM/#XF";
    case IDT_IDX_EXCEPTION_VE:
        return "#VE";
    case IDT_IDX_EXCEPTION_CP:
        return "#CP";
    case IDT_IDX_EXCEPTION_HV:
        return "#HV";
    case IDT_IDX_EXCEPTION_VC:
        return "#VC";
    case IDT_IDX_EXCEPTION_SX:
        return "#SX";
    default:
        return nullptr;
    }
}

static void legacy_pic_init_and_disable(uint8_t master_offset, uint8_t slave_offset) {
    // ICW1: Begin INIT sequence, including ICW4
    outb(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    // ICW2: Map interrupts to IDT vectors
    outb(PIC1_DATA, master_offset);
    outb(PIC2_DATA, slave_offset);

    // ICW3: Configure cascade
    outb(PIC1_DATA, 4); // Slave PIC at IRQ2 (mask)
    outb(PIC2_DATA, 2); // Slave PIC is IRQ2 (value)

    // ICW4: Enable 8086 mode
    outb(PIC1_DATA, PIC_ICW4_8086);
    outb(PIC2_DATA, PIC_ICW4_8086);

    // Mask all interrupts
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

static volatile uint32_t *xapic1_get_register(uint8_t reg) {
    return (void *)((uintptr_t)LAPIC_BASE + reg * LAPIC_REGISTER_SIZE);
}

static uint32_t lapic_read(uint8_t reg) {
    if (reg > LAPIC_REGISTER_MAX) panic("lapic_read: LAPIC Register 0x%x out of range", reg);

    if (HAS_X2APIC) {
        return rdmsr(MSR_X2APIC_BASE + reg);
    } else {
        return *xapic1_get_register(reg);
    }
}

static void lapic_write(uint8_t reg, uint32_t value) {
    if (reg > LAPIC_REGISTER_MAX) panic("lapic_write: LAPIC Register 0x%x out of range", reg);
    if (HAS_X2APIC) {
        wrmsr(MSR_X2APIC_BASE + reg, value);
    } else {
        *xapic1_get_register(reg) = value;
    }
}

static void lapic_eoi(void) {
    lapic_write(LAPIC_REGISTER_IDX_EOI, 0);
}

static void lapic_init(void) {
    const struct cpuid_result cpuid_result = cpuid(1, 0);
    HAS_X2APIC = !!(cpuid_result.ecx & CPUID_1_ECX_X2APIC);

    uint64_t lapic_base = rdmsr(MSR_IA32_APIC_BASE);

    if (!(lapic_base & MSR_IA32_APIC_BASE_EN)) {
        lapic_base |= MSR_IA32_APIC_BASE_EN;
        wrmsr(MSR_IA32_APIC_BASE, lapic_base);
    }
    if (HAS_X2APIC) {
        if (!(lapic_base & MSR_IA32_APIC_BASE_EXTD)) {
            lapic_base |= MSR_IA32_APIC_BASE_EXTD;
            wrmsr(MSR_IA32_APIC_BASE, lapic_base);
        }
    } else {
        const phy_t lapic_base_phy = lapic_base & MSR_IA32_APIC_BASE_MASK;
        void *lapic_base_virt = phy_to_virt(lapic_base_phy);
        vmm_map_unaligned(lapic_base_phy, lapic_base_virt, LAPIC_REGISTER_MAX * LAPIC_REGISTER_SIZE, M_W);
        LAPIC_BASE = lapic_base_virt;
    }

    lapic_write(LAPIC_REGISTER_IDX_LVT_TIMER, LAPIC_LVT_MASK);
    lapic_write(LAPIC_REGISTER_IDX_LVT_THERM, LAPIC_LVT_MASK);
    lapic_write(LAPIC_REGISTER_IDX_LVT_PERFC, LAPIC_LVT_MASK);
    if (LAPIC_NMI_REDIRECTION.lint == 0) {
        lapic_write(LAPIC_REGISTER_IDX_LVT_LINT0, LAPIC_LVT_DELIVERY_NMI);
        lapic_write(LAPIC_REGISTER_IDX_LVT_LINT1, LAPIC_LVT_DELIVERY_EXTINT);
    } else {
        lapic_write(LAPIC_REGISTER_IDX_LVT_LINT0, LAPIC_LVT_DELIVERY_EXTINT);
        lapic_write(LAPIC_REGISTER_IDX_LVT_LINT1, LAPIC_LVT_DELIVERY_NMI);
    }
    lapic_write(LAPIC_REGISTER_IDX_LVT_ERROR, LAPIC_LVT_MASK);
    lapic_write(LAPIC_REGISTER_IDX_SPIV, LAPIC_SPIV_ENABLE | 0xFF);
}

static uint32_t ioapic_read32(uint8_t reg) {
    *(volatile uint32_t *)IOAPIC_BASE = reg;
    return *(volatile uint32_t *)((uintptr_t)IOAPIC_BASE + 0x10);
}

static uint64_t ioapic_read64(uint8_t reg) {
    const uint32_t loval = ioapic_read32(reg);
    const uint32_t hival = ioapic_read32(reg + 1);

    return (((uint64_t)hival) << 32) | (uint64_t)loval;
}

static void ioapic_write32(uint8_t reg, uint32_t value) {
    *(volatile uint32_t *)IOAPIC_BASE = reg;
    *(volatile uint32_t *)((uintptr_t)IOAPIC_BASE + 0x10) = value;
}

static void ioapic_write64(uint8_t reg, uint64_t value) {
    ioapic_write32(reg, value & 0xFFFF'FFFF);
    ioapic_write32(reg + 1, value >> 32);
}

static void ioapic_enable_isa_interrupt(uint8_t vector) {
    const uint8_t i = vector - IDT_IDX_ISA_BASE;

    uint32_t value = IOAPICREDTBL_DELIVERY_FIXED(vector);
    if (ISA_REDIRECTION_ENTRY[i].is_active_low) {
        value |= IOAPICREDTBL_POLARITY;
    }
    if (ISA_REDIRECTION_ENTRY[i].is_level_triggered) {
        value |= IOAPICREDTBL_TRIGGER;
    }
    ioapic_write64(IOAPICREDTBL(ISA_REDIRECTION_ENTRY[i].destination), value);
}

static void ioapic_enable_nmi_interrupt() {
    uint32_t value = IOAPICREDTBL_DELIVERY_NMI;
    if (IOAPIC_NMI_REDIRECTION.is_active_low) {
        value |= IOAPICREDTBL_POLARITY;
    }
    if (IOAPIC_NMI_REDIRECTION.is_level_triggered) {
        value |= IOAPICREDTBL_TRIGGER;
    }
    ioapic_write64(IOAPICREDTBL(IOAPIC_NMI_REDIRECTION.destination), value);
}

static void ioapic_init() {
    const uint32_t ioapicver = ioapic_read32(IOAPICVER);
    const uint8_t num_ioapic_entries = (ioapicver >> IOAPICVER_MAXENTRY_SHIFT) & 0xFF;
    if (num_ioapic_entries < 16) panic("I/O APIC has too few pins to cover all ISA IRQs");

    for (uint8_t i = 0; i < num_ioapic_entries; ++i) {
        ioapic_write64(IOAPICREDTBL(i), IOAPICREDTBL_MASK);
    }

    if (IOAPIC_NMI_REDIRECTION.is_valid) {
        ioapic_enable_nmi_interrupt();
    }
    ioapic_enable_isa_interrupt(IDT_IDX_ISA_KB);
}

static void parse_madt(void) {
    const struct MADTEntryHeader *madt_entry;
    for (uint32_t i = 0; madt_entry = (const void *)&ACPI_MADT->records[i], i + offsetof(struct MADT, records) < ACPI_MADT->h.length; i += ((const struct MADTEntryHeader *)&ACPI_MADT->records[i])->length) {
        switch (madt_entry->type) {
        case 0: {
            const struct MADTProcessorLocalAPIC *madt_lapic = (const void *)madt_entry;
            if (madt_lapic->processor_id == 0) {
                // BSP
                if (madt_lapic->flags & 0x1) {
                    legacy_pic_init_and_disable(IDT_IDX_LEGACY_PIC_MASTER_BASE, IDT_IDX_LEGACY_PIC_SLAVE_BASE);
                }
            }
            break;
        }
        case 1: {
            const struct MADTIOAPIC *madt_ioapic =  (const void *)madt_entry;
            if (IOAPIC_BASE != 0) panic("Multiple I/O APICs not supported");
            if (madt_ioapic->global_system_interrupt_base != 0) panic("Remapping the entire I/O APIC is not supported");

            IOAPIC_BASE = phy_to_virt(madt_ioapic->address);
            vmm_map_unaligned(madt_ioapic->address, IOAPIC_BASE, 0x20, M_CACHE_UC | M_W);
            break;
        }
        case 2: {
            const struct MADTInterruptSourceOverride *madt_override = (const void *)madt_entry;
            if (madt_override->bus_source != 0) panic("Unknown MADT Bus %u", madt_override->bus_source);

            const uint8_t polarity = (madt_override->flags >> 0) & 0x3;
            const uint8_t trigger = (madt_override->flags >> 2) & 0x3;

            ISA_REDIRECTION_ENTRY[madt_override->irq_source].destination = madt_override->global_system_interrupt;
            ISA_REDIRECTION_ENTRY[madt_override->irq_source].is_active_low = polarity == 0x3;
            ISA_REDIRECTION_ENTRY[madt_override->irq_source].is_level_triggered = trigger == 0x3;
            break;
        }
        case 3:
            const struct MADTNMISource *madt_nmisource = (const void *)madt_entry;
            const uint8_t polarity = (madt_nmisource->flags >> 0) & 0x3;
            const uint8_t trigger = (madt_nmisource->flags >> 2) & 0x3;

            IOAPIC_NMI_REDIRECTION.destination = madt_nmisource->global_system_interrupt;
            IOAPIC_NMI_REDIRECTION.is_active_low = polarity == 0x3;
            IOAPIC_NMI_REDIRECTION.is_level_triggered = trigger == 0x3;
            IOAPIC_NMI_REDIRECTION.is_valid = true;
            break;
        case 4: {
            const struct MADTLocalAPICNMI *madt_localapicnmi = (const void *)madt_entry;
            if (madt_localapicnmi->processor_id == 0 || madt_localapicnmi->processor_id != 0xFF) {
                // BSP or ALL_CPUS
                const uint8_t polarity = (madt_localapicnmi->flags >> 0) & 0x3;
                const uint8_t trigger = (madt_localapicnmi->flags >> 2) & 0x3;

                LAPIC_NMI_REDIRECTION.lint = madt_localapicnmi->lint;
                LAPIC_NMI_REDIRECTION.is_active_low = polarity == 0x3;
                LAPIC_NMI_REDIRECTION.is_level_triggered= trigger == 0x3;
            }
            break;
        }
        default:
            panic("Unknown MADT struct %u", madt_entry->type);
        }
    }
}

void configure_interrupts(void) {
    parse_madt();

    lapic_init();
    ioapic_init();

    __asm("sti");
}

static void dump_interrupt_frame(const struct stack_frame *stack_frame) {
    kprint("r11: 0x%lx\n", stack_frame->r11);
    kprint("r10: 0x%lx\n", stack_frame->r10);
    kprint("r9: 0x%lx\n", stack_frame->r9);
    kprint("r8: 0x%lx\n", stack_frame->r8);
    kprint("rsi: 0x%lx\n", stack_frame->rsi);
    kprint("rdx: 0x%lx\n", stack_frame->rdx);
    kprint("rcx: 0x%lx\n", stack_frame->rcx);   
    kprint("rax: 0x%lx\n", stack_frame->rax);
    kprint("rdi: 0x%lx\n", stack_frame->rdi);
    kprint("reserved: 0x%lx\n", stack_frame->reserved);
    kprint("error_code: 0x%lx\n", stack_frame->error_code);
    kprint("rip: 0x%lx\n", stack_frame->rip);
    kprint("cs: 0x%lx\n", stack_frame->cs);
    kprint("rflags: 0x%lx\n", stack_frame->rflags);
    kprint("rsp: 0x%lx\n", stack_frame->rsp);
    kprint("ss: 0x%lx\n", stack_frame->ss);
}

void interrupt_handler(uint8_t vector, const struct stack_frame *stack_frame) {
    enforce_smap();

    switch (vector) {
    case IDT_IDX_EXCEPTION_BP:
        dump_interrupt_frame(stack_frame);
        kprint("Continuing after #BP...\n");
        break;
    case IDT_IDX_EXCEPTION_PF:
        uintptr_t cr2;
        __asm("mov %%cr2,%0" : "=r"(cr2));
        dump_interrupt_frame(stack_frame);
        panic("Unhandled #PF at 0x%lx", cr2);
    case IDT_IDX_LEGACY_PIC_MASTER_BASE + 0:
    case IDT_IDX_LEGACY_PIC_MASTER_BASE + 1:
    case IDT_IDX_LEGACY_PIC_MASTER_BASE + 2:
    case IDT_IDX_LEGACY_PIC_MASTER_BASE + 3:
    case IDT_IDX_LEGACY_PIC_MASTER_BASE + 4:
    case IDT_IDX_LEGACY_PIC_MASTER_BASE + 5:
    case IDT_IDX_LEGACY_PIC_MASTER_BASE + 6:
    case IDT_IDX_LEGACY_PIC_MASTER_BASE + 7:
        kprint("Spurious Interrupt 0x%x (Legacy PIC Master)", vector);
        break;
    case IDT_IDX_LEGACY_PIC_SLAVE_BASE + 0:
    case IDT_IDX_LEGACY_PIC_SLAVE_BASE + 1:
    case IDT_IDX_LEGACY_PIC_SLAVE_BASE + 2:
    case IDT_IDX_LEGACY_PIC_SLAVE_BASE + 3:
    case IDT_IDX_LEGACY_PIC_SLAVE_BASE + 4:
    case IDT_IDX_LEGACY_PIC_SLAVE_BASE + 5:
    case IDT_IDX_LEGACY_PIC_SLAVE_BASE + 6:
    case IDT_IDX_LEGACY_PIC_SLAVE_BASE + 7:
        kprint("Spurious Interrupt 0x%x (Legacy PIC Slave)", vector);
        break;
    case IDT_IDX_ISA_KB:
        kprint("Ignoring Interrupt from KB\n");
        lapic_eoi();
        break;
    case IDT_IDX_LAPIC_SPURIOUS:
        kprint("Spurious Interrupt (LAPIC)");
        // Do NOT send EOI. SDM 12.9: Spurious Interrupt
        break;          
    default:
        dump_interrupt_frame(stack_frame);
        const char *mnemonic = vector_to_menemonic(vector);
        if (mnemonic) {
            panic("Unhandled Exception %s", mnemonic);
        } else if (vector < 32) {
            panic("Unhandled Exception %d", vector);
        } else {
            panic("Unhandled Interrupt %d", vector);
        }
    }
}

#define I(X) \
    IDT[X].offset_low = (((uintptr_t)&interrupt_stub_##X) >> 0) & 0xFFFF; \
    IDT[X].offset_high = (((uintptr_t)&interrupt_stub_##X) >> 16) & 0xFFFF'FFFF'FFFF;

void load_idt(void) {
    FOR_EACH_INTERRUPT

    const struct dtr idtr = {
        .limit = sizeof(IDT) - 1,
        .addr = (uintptr_t)&IDT
    };
    __asm("lidt %0" : : "m"(idtr));
}

#undef I
