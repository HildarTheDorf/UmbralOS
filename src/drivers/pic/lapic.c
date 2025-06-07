#include "lapic.h"

#include "common.h"
#include "intel.h"
#include "mm.h"

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

static bool HAS_X2APIC;
static void *LAPIC_BASE;

struct LAPICNMIRedirectionInfo {
    uint8_t lint;
    bool is_level_triggered;
    bool is_active_low;
    bool is_valid;
} LAPIC_NMI_REDIRECTION;

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

void lapic_eoi(void) {
    lapic_write(LAPIC_REGISTER_IDX_EOI, 0);
}

void lapic_init_begin(void) {
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
    lapic_write(LAPIC_REGISTER_IDX_LVT_LINT0, LAPIC_LVT_MASK);
    lapic_write(LAPIC_REGISTER_IDX_LVT_LINT1, LAPIC_LVT_MASK);
    lapic_write(LAPIC_REGISTER_IDX_LVT_ERROR, LAPIC_LVT_MASK);
}

void lapic_init_nmi(const struct MADTLocalAPICNMI *madt_localapicnmi) {
    if (madt_localapicnmi->processor_id == 0 || madt_localapicnmi->processor_id == 0xFF) {
        // BSP or ALL_CPUS
        const uint8_t polarity = (madt_localapicnmi->flags >> 0) & 0x3;
        const uint8_t trigger = (madt_localapicnmi->flags >> 2) & 0x3;

        LAPIC_NMI_REDIRECTION.lint = madt_localapicnmi->lint;
        LAPIC_NMI_REDIRECTION.is_active_low = polarity == 0x3;
        LAPIC_NMI_REDIRECTION.is_level_triggered= trigger == 0x3;
        LAPIC_NMI_REDIRECTION.is_valid = true;
    }
}

void lapic_init_finalize(void) {
    if (LAPIC_NMI_REDIRECTION.is_valid) {
        if (LAPIC_NMI_REDIRECTION.lint == 0) {
            lapic_write(LAPIC_REGISTER_IDX_LVT_LINT0, LAPIC_LVT_DELIVERY_NMI);
        } else {
            lapic_write(LAPIC_REGISTER_IDX_LVT_LINT1, LAPIC_LVT_DELIVERY_NMI);
        }
    }

    lapic_write(LAPIC_REGISTER_IDX_SPIV, LAPIC_SPIV_ENABLE | 0xFF);
}
