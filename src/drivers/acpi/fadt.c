#include "internal.h"

#include "common.h"

#define HW_REDUCED_ACPI (1 << 20)
#define IAPC_BOOT_ARCH_8042 (1 << 1)

void acpi_parse_fadt(const struct FADT *pFADT) {
    if (!(pFADT->Flags & HW_REDUCED_ACPI) && (pFADT->BootArchitectureFlags & IAPC_BOOT_ARCH_8042)) {
        kprint("PS/2 HID is not emulated");
    }
}
