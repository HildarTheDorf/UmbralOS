#include "common.h"
#include "gdt.h"
#include "interrupt.h"
#include "mm.h"
#include "security.h"
#include "serial.h"

#define DEFAULT_STACK_SIZE 0x10000

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


[[noreturn]]
void main(void *stack_origin) {
    security_init();
    serial_init();

    load_gdt();
    load_idt();

    pmm_init(limine_memmap_request.response, (void *)limine_hhdm_request.response->offset);
    vmm_init(limine_memmap_request.response, limine_kernel_address_request.response);
    pmm_reclaim(limine_memmap_request.response, stack_origin, DEFAULT_STACK_SIZE);
    pmm_zero(); // TODO: Disable this in release builds

    kprint("Boot Complete!\n");
    halt();
}
