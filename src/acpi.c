#include "acpi.h"

#include "common.h"
#include "mm.h"

#include <stdint.h>

struct [[gnu::packed]] RSDP {
    char signature[8];
    uint8_t checksum;
    char OEMID[6];
    uint8_t revision;
    phy32_t rsdt_address;
};

struct [[gnu::packed]] XSDP {
    struct RSDP rsdp;

    uint32_t length;
    phy_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
};

struct [[gnu::packed]] SDTHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};

struct [[gnu::packed]] RSDT {
    struct SDTHeader h;
    phy32_t sdt32[];
};

struct [[gnu::packed]] XSDT {
    struct SDTHeader h;
    phy_t sdt64[];
};

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

struct [[gnu::packed]] MADTLocalAPICNMI {
    struct MADTEntryHeader h;
    uint8_t processor_id;
    uint16_t flags;
    uint8_t lint;
};

static void validate_sdt(const struct SDTHeader *pSDT) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < pSDT->length; ++i) {
        checksum += ((const uint8_t *)pSDT)[i];
    }
    if (checksum) panic("Invalid SDT. Expected 0, got %u\n", checksum);
}

static void validate_rsdp(const struct RSDP *pRSDP) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < sizeof(struct RSDP); ++i) {
        checksum += ((const uint8_t *)pRSDP)[i];
    }
    if (checksum) panic("Invalid RSDP. Expected 0, got %u\n", checksum);
}

static void validate_xsdp(const struct XSDP *pXSDP) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < sizeof(struct XSDP); ++i) {
        checksum += ((const uint8_t *)pXSDP)[i];
    }
    if (checksum) panic("Invalid XSDP. Expected 0, got %u\n", checksum);
}

static void parse_madt(const struct MADT *pMADT) {
    for (uint32_t i = 0; i + offsetof(struct MADT, records) < pMADT->h.length; i += ((const struct MADTEntryHeader *)&pMADT->records[i])->length) {
        const struct MADTEntryHeader *pHeader = (const void *)&pMADT->records[i];
        switch (pHeader->type) {
        case 0: // Processor Local APIC
            const struct MADTProcessorLocalAPIC *pProcessorLocalAPIC = (const void *)pHeader;
            kprint("Local APIC for processor %u: %u(0x%x)\n", pProcessorLocalAPIC->processor_id, pProcessorLocalAPIC->apic_id, pProcessorLocalAPIC->flags);
            break;
        case 1: // I/O APIC
            const struct MADTIOAPIC *pIOAPIC = (const void *)pHeader;
            kprint("I/O APIC #%u@0x%x(0x%x)\n", pIOAPIC->apic_id, pIOAPIC->address, pIOAPIC->global_system_interrupt_base);
            break;
        case 2: // Interrupt Source Override
            const struct MADTInterruptSourceOverride *pInterruptSourceOverride = (const void *)pHeader;
            kprint("Interrupt Source Override BUS %u IRQ 0x%x: 0x%x(0x%x)\n", pInterruptSourceOverride->bus_source, pInterruptSourceOverride->irq_source, pInterruptSourceOverride->global_system_interrupt, pInterruptSourceOverride->flags);
            break;
        case 4: // Local APIC NMI
            const struct MADTLocalAPICNMI *pLocalAPICNMI = (const void *)pHeader;
            kprint("Non-Maskable local interrupt CPU #%u: LINT_#%u(0x%x)\n", pLocalAPICNMI->processor_id, pLocalAPICNMI->lint, pLocalAPICNMI->flags);
            break;
        default:
            panic("Unknown MADT Entry 0x%x", pHeader->type);
        }
    }
}

static void parse_sdt(const struct SDTHeader *pSDT) {
    validate_sdt(pSDT);
    if (!strncmp(pSDT->signature, "APIC", 4)) {
        parse_madt((const struct MADT *)pSDT);
    }
}

void acpi_parse_rsdp(const void *pRSDP) {
    const struct XSDP *pXSDP = pRSDP;
    validate_rsdp(&pXSDP->rsdp);

    bool has_xsdp = pXSDP->rsdp.revision > 0;
    if (has_xsdp) {
        validate_xsdp(pXSDP);

        const struct XSDT *pXSDT = phy_to_virt(pXSDP->xsdt_address);
        validate_sdt(&pXSDT->h);

        const size_t num_sdts = (pXSDT->h.length - offsetof(struct XSDT, sdt64)) / sizeof(phy_t);
        for (size_t i = 0; i < num_sdts; ++i) {
            parse_sdt(phy_to_virt(pXSDT->sdt64[i]));
        }
    } else {
        const struct RSDT *pRSDT = phy_to_virt(pXSDP->rsdp.rsdt_address);
        validate_sdt(&pRSDT->h);

        const size_t num_sdts = (pRSDT->h.length - offsetof(struct RSDT, sdt32)) / sizeof(phy32_t);
        for (size_t i = 0; i < num_sdts; ++i) {
            parse_sdt(phy_to_virt(pRSDT->sdt32[i]));        
        }
    }
}
