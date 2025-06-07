#pragma once

#include "drivers/acpi/madt.h"

void ioapic_enable_isa_interrupt(uint8_t vector);
void ioapic_init_register(const struct MADTIOAPIC *madt_ioapic);
void ioapic_init_nmisource(const struct MADTNMISource *madt_nmisource);
void ioapic_init_source_override(const struct MADTInterruptSourceOverride *madt_override);
