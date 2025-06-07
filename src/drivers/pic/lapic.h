#pragma once

#include "drivers/acpi/madt.h"

void lapic_eoi(void);

void lapic_init_begin(void);
void lapic_init_nmi(const struct MADTLocalAPICNMI *madt_localapicnmi);
void lapic_init_finalize(void);
