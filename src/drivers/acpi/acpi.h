#pragma once

#include <stdint.h>

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

struct [[gnu::packed]] GenericAddress
{
    uint8_t AddressSpace;
    uint8_t BitWidth;
    uint8_t BitOffset;
    uint8_t AccessSize;
    uint64_t Address;
};

void acpi_parse_rsdp(const void *p);
