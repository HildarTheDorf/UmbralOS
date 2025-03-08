#include "common.h"
#include "gdt.h"
#include "interrupt.h"
#include "serial.h"

#include <limine.h>

LIMINE_REQUESTS_START_MARKER
LIMINE_BASE_REVISION(3)

struct limine_hhdm_request limine_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST
};

struct limine_kernel_address_request limine_kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST
};

struct limine_memmap_request limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,  
};

LIMINE_REQUESTS_END_MARKER

static const char *memmap_type_to_string(int type) {
    switch (type) {
    case LIMINE_MEMMAP_USABLE:
        return "Usable";
    case LIMINE_MEMMAP_RESERVED:
        return "Reserved";
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return "ACPI (reclaimable)";
    case LIMINE_MEMMAP_ACPI_NVS:
        return "ACPI (NVS)";
    case LIMINE_MEMMAP_BAD_MEMORY:
        return "Bad";
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        return "Bootloader (reclaimable)";
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:
        return "Kernel/Modules";
    case LIMINE_MEMMAP_FRAMEBUFFER:
        return "Framebuffer";
    default:
        return "UNKNOWN";
    }
}

[[noreturn]]
void main(void *stack_origin) {
    serial_init();

    load_gdt();
    load_idt();

    for (unsigned i = 0; i < limine_memmap_request.response->entry_count; ++i) {
        const struct limine_memmap_entry *entry = limine_memmap_request.response->entries[i];
        kprint("Memmap entry %u(%s): 0x%lx-0x%lx\n", i, memmap_type_to_string(entry->type), entry->base, entry->base + entry->length);
    }
    kprint("HHDM Address: 0x%lx\n", limine_hhdm_request.response->offset);
    kprint("Kernel Address:\n    Physical: 0x%lx\n    Virtual: 0x%lx\n", limine_kernel_address_request.response->physical_base, limine_kernel_address_request.response->virtual_base);
    kprint("64KiB bytes of stack at 0x%p\n", stack_origin);

    kprint("Boot Complete!\n");
    halt();
}
