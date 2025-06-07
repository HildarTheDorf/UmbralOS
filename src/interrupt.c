#include "common.h"
#include "drivers/pic/ioapic.h"
#include "drivers/pic/lapic.h"
#include "intel.h"
#include "security.h"
#include "serial.h"

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

void disable_interrupts(void) {
    __asm("cli");
}

void enable_interrupts(void) {
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
        kprint("Got Interrupt from PS/2 KB (0x%u)\n", inb(0x60));
        lapic_eoi();
        break;
    case IDT_IDX_ISA_COM1:
        const char c = serial_read();
        if (c == '\r') {
            kprint("\n");
        } else {
            kprint("%c", c);
        }
        lapic_eoi();
        break;
    case IDT_IDX_ISA_MOUSE:
        kprint("Got Interrupt from PS/2 Mouse (0x%u)\n", inb(0x60));
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
            panic("Unhandled Exception 0x%x", vector);
        } else {
            panic("Unhandled Interrupt 0x%x", vector);
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
