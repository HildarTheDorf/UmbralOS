#include "drivers/acpi/acpi.h"
#include "internal.h"

#include "common.h"

#define HW_REDUCED_ACPI (1 << 20)
#define IAPC_BOOT_ARCH_8042 (1 << 1)

static bool has_i8042 = true;

bool acpi_has_legacy_feature(enum AcpiLegacyFeature feature) {
    switch(feature) {
    case ACPI_LEGACY_FEATURE_i8042:
        return has_i8042;
    default:
        panic("Unknown AcpiLegacyFeatue '%u'", feature);
    }
}

void acpi_parse_fadt(const struct FADT *pFADT) {
    if (pFADT->Flags & HW_REDUCED_ACPI) {
        kprint("ACPI Legacy Hardware not present");
        has_i8042 = false;
    }
    if (pFADT->BootArchitectureFlags & IAPC_BOOT_ARCH_8042) {
        kprint("ACPI reports i8042 present");
        has_i8042 = true;
    }
}
