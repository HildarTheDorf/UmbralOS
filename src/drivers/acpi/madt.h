#pragma once

#include "acpi.h"
#include "mm.h"

struct [[gnu::packed]] MADT {
    struct SDTHeader h;
    phy32_t lapic_addr;
    uint32_t flags;
    char records[];
};

struct [[gnu::packed]] MADTEntryHeader {
    uint8_t type;
    uint8_t length;
};

struct [[gnu::packed]] MADTProcessorLocalAPIC {
    struct MADTEntryHeader h;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
};

struct [[gnu::packed]] MADTIOAPIC {
    struct MADTEntryHeader h;
    uint8_t apic_id;
    uint8_t reserved;
    uint32_t address;
    uint32_t global_system_interrupt_base;
};

struct [[gnu::packed]] MADTInterruptSourceOverride {
    struct MADTEntryHeader h;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
};

struct [[gnu::packed]] MADTNMISource {
    struct MADTEntryHeader h;
    uint16_t flags;
    uint32_t global_system_interrupt;
};

struct [[gnu::packed]] MADTLocalAPICNMI {
    struct MADTEntryHeader h;
    uint8_t processor_id;
    uint16_t flags;
    uint8_t lint;
};

