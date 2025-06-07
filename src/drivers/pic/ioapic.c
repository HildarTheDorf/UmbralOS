#include "ioapic.h"

#include "8259.h"
#include "common.h"
#include "drivers/acpi/madt.h"
#include "intel.h"

#define IOAPICVER 0x01
#define IOAPIC_REDIR_TBL_ADDR(n) (0x10 + 2 * n)

#define IOAPICVER_MAXENTRY_SHIFT 16

#define IOAPIC_REDIR_TBL_POLARITY (1 << 13)
#define IOAPIC_REDIR_TBL_TRIGGER (1 << 15)
#define IOAPIC_REDIR_TBL_MASKED (1 << 16)

#define IOAPIC_REDIR_TBL_DELIVERY_FIXED(vector) (vector)
#define IOAPIC_REDIR_TBL_DELIVERY_NMI (0b100 << 8)

struct redirection_entry {
    uint8_t destination;
    bool is_level_triggered;
    bool is_active_low;    
};

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
    uint16_t destination;
    bool is_level_triggered;
    bool is_active_low;
    bool is_valid;
} IOAPIC_NMI_REDIRECTION;

static uint32_t ioapic_read32(uint8_t reg) {
    // IOAPIC registers are 16-byte aligned but should be accessed using 32-bit ops
    *(volatile uint32_t *)IOAPIC_BASE = reg;
    return *(volatile uint32_t *)((uintptr_t)IOAPIC_BASE + 0x10);
}

static uint64_t ioapic_read64(uint8_t reg) {
    const uint32_t loval = ioapic_read32(reg);
    const uint32_t hival = ioapic_read32(reg + 1);

    return (((uint64_t)hival) << 32) | (uint64_t)loval;
}

static void ioapic_write32(uint8_t reg, uint32_t value) {
    // IOAPIC registers are 16-byte aligned but should be accessed using 32-bit ops
    *(volatile uint32_t *)IOAPIC_BASE = reg;
    *(volatile uint32_t *)((uintptr_t)IOAPIC_BASE + 0x10) = value;
}

static void ioapic_write64(uint8_t reg, uint64_t value) {
    ioapic_write32(reg, value & 0xFFFF'FFFF);
    ioapic_write32(reg + 1, value >> 32);
}

static void ioapic_disable_interrupt(uint8_t vector) {
    ioapic_write64(IOAPIC_REDIR_TBL_ADDR(vector), IOAPIC_REDIR_TBL_MASKED);
}

static void ioapic_enable_nmi_interrupt(void) {
    uint32_t value = IOAPIC_REDIR_TBL_DELIVERY_NMI;
    if (IOAPIC_NMI_REDIRECTION.is_active_low) {
        value |= IOAPIC_REDIR_TBL_POLARITY;
    }
    if (IOAPIC_NMI_REDIRECTION.is_level_triggered) {
        value |= IOAPIC_REDIR_TBL_TRIGGER;
    }
    ioapic_write64(IOAPIC_REDIR_TBL_ADDR(IOAPIC_NMI_REDIRECTION.destination), value);
}

static void ioapic_enable_isa_interrupt(uint8_t vector) {
    const uint8_t i = vector - IDT_IDX_ISA_BASE;

    uint32_t value = IOAPIC_REDIR_TBL_DELIVERY_FIXED(vector);
    if (ISA_REDIRECTION_ENTRY[i].is_active_low) {
        value |= IOAPIC_REDIR_TBL_POLARITY;
    }
    if (ISA_REDIRECTION_ENTRY[i].is_level_triggered) {
        value |= IOAPIC_REDIR_TBL_TRIGGER;
    }
    ioapic_write64(IOAPIC_REDIR_TBL_ADDR(ISA_REDIRECTION_ENTRY[i].destination), value);
}

void ioapic_init_register(const struct MADTIOAPIC *madt_ioapic) {
    if (IOAPIC_BASE != 0) panic("Multiple I/O APICs not supported");
    if (madt_ioapic->global_system_interrupt_base != 0) panic("Remapping the entire I/O APIC is not supported");

    IOAPIC_BASE = phy_to_virt(madt_ioapic->address);
    vmm_map_unaligned(madt_ioapic->address, IOAPIC_BASE, 0x20, M_CACHE_UC | M_W);
}

void ioapic_init_nmisource(const struct MADTNMISource *madt_nmisource) {
    const uint8_t polarity = (madt_nmisource->flags >> 0) & 0x3;
    const uint8_t trigger = (madt_nmisource->flags >> 2) & 0x3;

    IOAPIC_NMI_REDIRECTION.destination = madt_nmisource->global_system_interrupt;
    IOAPIC_NMI_REDIRECTION.is_active_low = polarity == 0x3;
    IOAPIC_NMI_REDIRECTION.is_level_triggered = trigger == 0x3;
    IOAPIC_NMI_REDIRECTION.is_valid = true;
}

void ioapic_init_source_override(const struct MADTInterruptSourceOverride *madt_override) {
    if (madt_override->bus_source != 0) panic("Unknown MADT Bus %u", madt_override->bus_source);

    const uint8_t polarity = (madt_override->flags >> 0) & 0x3;
    const uint8_t trigger = (madt_override->flags >> 2) & 0x3;

    ISA_REDIRECTION_ENTRY[madt_override->irq_source].destination = madt_override->global_system_interrupt;
    ISA_REDIRECTION_ENTRY[madt_override->irq_source].is_active_low = polarity == 0x3;
    ISA_REDIRECTION_ENTRY[madt_override->irq_source].is_level_triggered = trigger == 0x3;
}

void ioapic_init_finalize(void) {
    const uint32_t ioapicver = ioapic_read32(IOAPICVER);
    const uint8_t num_ioapic_entries = (ioapicver >> IOAPICVER_MAXENTRY_SHIFT) & 0xFF;
    if (num_ioapic_entries < 16) panic("I/O APIC has too few pins to cover all ISA IRQs");

    for (uint8_t i = 0; i < num_ioapic_entries; ++i) {
        ioapic_disable_interrupt(i);
    }

    if (IOAPIC_NMI_REDIRECTION.is_valid) {
        ioapic_enable_nmi_interrupt();
    }
    ioapic_enable_isa_interrupt(IDT_IDX_ISA_KB);
    ioapic_enable_isa_interrupt(IDT_IDX_ISA_COM1);
    ioapic_enable_isa_interrupt(IDT_IDX_ISA_MOUSE);
}
