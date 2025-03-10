#include "common.h"
#include "gdt.h"
#include "interrupt.h"
#include "mm.h"
#include "serial.h"

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

    pmm_init(limine_memmap_request.response, (void *)limine_hhdm_request.response->offset);
    vmm_init(limine_memmap_request.response, limine_kernel_address_request.response);

    kprint("Boot Complete!\n");
    halt();
}
