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

struct cpuid_result {
    uint64_t eax, ebx, ecx, edx;
};

static struct cpuid_result cpuid(uint32_t eax, uint32_t ecx) {
    struct cpuid_result ret;
    __asm("cpuid"
        : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx)
        : "a"(eax), "c"(ecx));
    return ret;
}

static void security_init(void) {
    const struct cpuid_result cpuid70 = cpuid(7,0);
    const bool has_smap = cpuid70.ebx & (1 << 20);
    const bool has_smep = cpuid70.ebx & (1 << 7);
    const bool has_umip = cpuid70.ecx & (1 << 2);

    uint64_t cr4;
    __asm volatile("mov %%cr4,%0" : "=r"(cr4));
    if (has_smap) cr4 |= (1 << 21);
    if (has_smep) cr4 |= (1 << 20);
    if (has_umip) cr4 |= (1 << 11);
    __asm("mov %0,%%cr4" : : "r"(cr4));

    if (has_smap) {
        __asm("clac");
    }
}

[[noreturn]]
void main(void *stack_origin) {
    security_init();
    serial_init();

    load_gdt();
    load_idt();

    pmm_init(limine_memmap_request.response, (void *)limine_hhdm_request.response->offset);
    vmm_init(limine_memmap_request.response, limine_kernel_address_request.response);

    kprint("Boot Complete!\n");
    halt();
}
