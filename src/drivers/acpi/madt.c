#include "madt.h"
#include "internal.h"

#include "common.h"
#include "drivers/pic/8259.h"
#include "drivers/pic/ioapic.h"
#include "drivers/pic/lapic.h"
#include "intel.h"

#define PCAT_COMPAT (1 << 0)

void acpi_parse_madt(const struct MADT *pMADT) {
    lapic_init_begin();

    if (pMADT->flags & PCAT_COMPAT) {
        legacy_pic_init_and_disable(IDT_IDX_LEGACY_PIC_MASTER_BASE, IDT_IDX_LEGACY_PIC_SLAVE_BASE);
    }

    const struct MADTEntryHeader *madt_entry;
    for (uint32_t i = 0; madt_entry = (const void *)&pMADT->records[i], i + offsetof(struct MADT, records) < pMADT->h.length; i += ((const struct MADTEntryHeader *)&pMADT->records[i])->length) {
        switch (madt_entry->type) {
        case 0: 
            // Processor Local APIC
            break;
        case 1: {
            const struct MADTIOAPIC *madt_ioapic =  (const void *)madt_entry;
            ioapic_init_register(madt_ioapic);
            break;
        }
        case 2: {
            const struct MADTInterruptSourceOverride *madt_override = (const void *)madt_entry;
            ioapic_init_source_override(madt_override);
            break;
        }
        case 3: {
            const struct MADTNMISource *madt_nmisource = (const void *)madt_entry;
            ioapic_init_nmisource(madt_nmisource);
            break;
        }
        case 4: {
            const struct MADTLocalAPICNMI *madt_localapicnmi = (const void *)madt_entry;
            lapic_init_nmi(madt_localapicnmi);
            break;
        }
        default:
            panic("Unknown MADT struct %u", madt_entry->type);
        }
    }
}
