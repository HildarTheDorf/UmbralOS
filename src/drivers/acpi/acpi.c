#include "internal.h"

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
    uint8_t checksum;
    uint8_t reserved[3];
};

struct [[gnu::packed]] RSDT {
    struct SDTHeader h;
    phy32_t sdt32[];
};

struct [[gnu::packed]] XSDT {
    struct SDTHeader h;
    phy_t sdt64[];
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

static void parse_sdt(const struct SDTHeader *pSDT) {
    validate_sdt(pSDT);

    const char *const signature = pSDT->signature;
    if (!strncmp(signature, "APIC", 4)) {
        acpi_parse_madt((const void *)pSDT);
    } else if (!strncmp(signature, "FACP", 4)) {
        acpi_parse_fadt((const void *)pSDT);
    }
}

void acpi_parse_rsdp(const void *p) {
    const struct RSDP *pRSDP = p;
    validate_rsdp(pRSDP);

    bool has_xsdp = pRSDP->revision > 0;
    if (has_xsdp) {
        const struct XSDP *pXSDP = p;
        validate_xsdp(pXSDP);

        const struct XSDT *pXSDT = phy_to_virt(pXSDP->xsdt_address);
        validate_sdt(&pXSDT->h);

        const size_t num_sdts = (pXSDT->h.length - offsetof(struct XSDT, sdt64)) / sizeof(phy_t);
        for (size_t i = 0; i < num_sdts; ++i) {
            parse_sdt(phy_to_virt(pXSDT->sdt64[i]));
        }
    } else {
        const struct RSDT *pRSDT = phy_to_virt(pRSDP->rsdt_address);
        validate_sdt(&pRSDT->h);

        const size_t num_sdts = (pRSDT->h.length - offsetof(struct RSDT, sdt32)) / sizeof(phy32_t);
        for (size_t i = 0; i < num_sdts; ++i) {
            parse_sdt(phy_to_virt(pRSDT->sdt32[i]));        
        }
    }
}
