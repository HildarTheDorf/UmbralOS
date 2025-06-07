#pragma once

#include "fadt.h"
#include "madt.h"

void acpi_parse_fadt(const struct FADT *pFADT);
void acpi_parse_madt(const struct MADT *pMADT);
